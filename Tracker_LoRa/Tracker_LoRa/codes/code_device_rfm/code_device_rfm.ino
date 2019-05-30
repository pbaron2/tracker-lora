#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>



#define DEBUG

#define uS_TO_S_FACTOR 1000000



#define GPS_BAUDRATE 9600L
#define GPS_RX 16
#define GPS_TX 17

#define PIN_BAT 33


RTC_DATA_ATTR int bootCount = 0;


// GPS
HardwareSerial uart2(2);
TinyGPSPlus gps;


byte batterie = 0;
byte freq_act = 1080;

#ifdef DEBUG
#define LED_PIN 22 // LED_BUILTIN
#endif


// LoRa - RFM95
// MISO -- D12 / d19
// MOSI -- D11 / d23
// SCK  -- D13 / d18
#define RFM95_CS  5  // NSS
#define RFM95_RST 27  // RESET
#define RFM95_INT 26  // DI0

#define RF95_FREQ 868850000



void setup()
{

  // Mapping du reset du RFM
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, LOW);


#ifdef DEBUG
  // Liaison Serie Debug
  Serial.begin(115200);
  delay(10);

  Serial.println("\n\n\n\tBalise GPS LoRa - Debug\n\n");
#endif


  // Mapping du RFM
  LoRa.setPins(RFM95_CS, RFM95_RST, RFM95_INT);

#ifdef DEBUG
  pinMode(LED_PIN, OUTPUT);
#endif

  // Reset manuel du RFM
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);


  // Initialisation RFM
  if(!LoRa.begin(RF95_FREQ))
  {
#ifdef DEBUG
    Serial.println("LoRa radio init failed");
    return;
#endif
    ESP.restart();
  }
  
#ifdef DEBUG
  Serial.println("LoRa radio init OK!");
#endif

  // Parametrage RFM
  // 868.85MHz, 23dBm, Bw = 125 kHz, Cr = 4/5, Sf = 4096chips/symbol, CRC off (manual)
  LoRa.setSpreadingFactor(12);
  LoRa.setPreambleLength(4);
  LoRa.setCodingRate4(5);
  LoRa.setSignalBandwidth(125000);
  LoRa.setTxPower(23, true);
  LoRa.disableCrc();
  LoRa.sleep();
  

#ifdef DEBUG
    Serial.println("Init LoRa OK");
#endif

  // Initialisatation UART 
  uart2.begin(GPS_BAUDRATE, SERIAL_8N1, GPS_RX, GPS_TX);
  //uart2.flush();
  //uart2.write("$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28");
  
  
#ifdef DEBUG
    Serial.println("Init GPS OK");
#endif

  // Mesure batterie
  batterie = analogRead(PIN_BAT) / 16;
#ifdef DEBUG
  Serial.print("Mesure batterie : ");
  Serial.println(batterie);
#endif

/*
  for(int wait = 0; wait < 1000; wait++)
{
  int packetSize = LoRa.parsePacket();
  if (packetSize)
  {
      // received a packet
#ifdef DEBUG
      Serial.println(wait);
      Serial.print("\tReceived packet '");
#endif
  
      // read packet
      while (LoRa.available())
      {
        freq_act = (byte)LoRa.read();
#ifdef DEBUG
        Serial.println(int(freq_act));
#endif
      }
  }
  delay(1);
  
}
*/
  
  bootCount++;
#ifdef DEBUG
  Serial.print("BootCount : ");
  Serial.println(bootCount);
#endif
}




void loop()
{                       
  //char radiopacket[10] = "\x1C\xAC\x72\x56\xFF\x0C\x19\x5E\x64";
  char radiopacket[11] = { 0 };

  while (uart2.available() > 0)
  {
    char c = uart2.read();
#ifdef DEBUG
    Serial.print(c);
#endif
    if (gps.encode(c))
    {

      if(gps.location.isValid() && gps.location.age() < 2000)
      {
        int32_t lat = (int32_t) (gps.location.lat() * 10000000);
        int32_t lon = (int32_t) (gps.location.lng() * 10000000);

#ifdef DEBUG
        Serial.print("\nLAT : ");
        Serial.print(gps.location.lat());
        Serial.print(" / ");
        Serial.println(lat);
        Serial.print("LON : ");
        Serial.print(gps.location.lng());
        Serial.print(" / ");
        Serial.println(lon);
        Serial.print("Age : ");
        Serial.println(gps.location.age());
#endif
        
    
        radiopacket[0] = (lat >> 24) &  0x000000FF;
        radiopacket[1] = (lat >> 16) &  0x000000FF;
        radiopacket[2] = (lat >> 8) &  0x000000FF;
        radiopacket[3] = lat &  0x000000FF;
        radiopacket[4] = (lon >> 24) &  0x000000FF;
        radiopacket[5] = (lon >> 16) &  0x000000FF;
        radiopacket[6] = (lon >> 8) &  0x000000FF;
        radiopacket[7] = lon &  0x000000FF;
        radiopacket[8] = batterie;
        
        byte crc = 0;
        for(char i = 0; i < 9; i++)
          crc += radiopacket[i];
        radiopacket[9] = crc;
        
        radiopacket[10] = '\0';

#ifdef DEBUG
        Serial.print("CRC : ");
        Serial.println(crc);
#endif
      
        LoRa.beginPacket();                   // start packet
        LoRa.print(radiopacket);              // add payload
        LoRa.endPacket();                     // finish packet and send it

        LoRa.rx_single();
        
#ifdef DEBUG
        Serial.print("SEND : ");
        for(char i = 0; i < 9; i++)
        {
          Serial.print(int(radiopacket[i]));
          Serial.print(" ");
        }
        Serial.print("\n");


#endif
        //delay(2000);

int wait = 0;
int packetSize = 0;
while( wait < 1800 && !packetSize)
{
  packetSize = LoRa.parsePacket();
  if (packetSize)
  {
      // received a packet
#ifdef DEBUG
      Serial.println(wait);
      Serial.print("\tReceived packet : ");
#endif
  
      // read packet
      while (LoRa.available())
      {
        //freq_act = (byte)LoRa.read() * 1080 / 255;
#ifdef DEBUG
        Serial.println(int(freq_act));
#endif
      }
  }
  wait++;
  delay(1);
  
}

        LoRa.sleep();
        
        //delay(10);
#ifdef DEBUG
        Serial.print("Sleep for : (s) ");
        Serial.println(freq_act);
#endif


        if(freq_act < 10)
        {
          changeUpdateTimeGPS((freq_act - 1) * 1000, false);
        }
        else
        {
          changeUpdateTimeGPS(freq_act * 0.95 * 1000, true);
        }
        
        esp_sleep_enable_timer_wakeup(freq_act * uS_TO_S_FACTOR);
        esp_deep_sleep_start();
      }
    }
  }


}



#ifdef DEBUG
void blinkLed(int range)
{
  digitalWrite(LED_PIN, LOW);
  delay(range);
  digitalWrite(LED_PIN, HIGH);
  delay(range);
}
#endif



void changeUpdateTimeGPS(int timeMS, bool modeOnOff)
{
  char str[52] = {0xB5, 0x62, 0x06, 0x3B, 0x2C, 0x00, 0x01, 0x06, 0x00, \
                  0x00, 0x0E, 0x10, 0x42, 0x01, timeMS & 0xFF, (timeMS >> 8) & 0xFF, (timeMS >> 16) & 0xFF, (timeMS >> 24) & 0xFF, \
                  0x10, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                  0x00, 0x00, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x4F, 0xC1, \
                  0x03, 0x00, 0x86, 0x02, 0x00, 0x00, 0xFE, 0x00, 0x00, \
                  0x00, 0x64, 0x40, 0x01, 0x00, 0x00, 0x00};

    if(modeOnOff)
      str[12] = 0x40;
    
    byte ck_a = 0;
    byte ck_b = 0;
    
    for(char i = 2; i < 50; i++)
    {
        ck_a = ck_a + str[i];
        ck_b = ck_b + ck_a;
    }
    str[50] = (char) ck_a;
    str[51] = (char) ck_b;

    uart2.write(str);
}



