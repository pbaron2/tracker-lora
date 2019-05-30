
#include <SoftwareSerial.h>
#include "main.h"
#include "E32-TTL-100.h"






SoftwareSerial softSerial(SOFT_RX, SOFT_TX);  // RX, TX




void setup()
{
  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);
  pinMode(AUX_PIN, INPUT);

  softSerial.begin(9600);
  Serial.begin(115200);
  Serial.println("BOOT...");

  initLora();
}


void loop()
{
  Serial.println("ENVOI");
  //uint8_t data[4] = {0xF0, 0xAA, 0x55, 0x0F};
  //uint8_t data[4] = {'a', 'B', 'c', 'D'};
  uint8_t data = 'a';
  
  SwitchMode(MODE_0_NORMAL);
  
  if(SendMsg(data) == RET_SUCCESS)
    Serial.println("ENVOI OK");
  else
    Serial.println("ENVOI ECHEC");
    
  SwitchMode(MODE_3_SLEEP);

  delay(500);
}



void initLora()
{
  RET_STATUS STATUS = RET_SUCCESS;
  struct CFGstruct CFG;
  struct MVerstruct MVer;
  
  STATUS = SleepModeCmd(R_CFG, (void* )&CFG); // Read config (0xC1)
  STATUS = SettingModule(&CFG);               // reply 6 bytes in return

  STATUS = SleepModeCmd(R_MODULE_VERSION, (void* )&MVer); // get module version (0xC3)

  
  SwitchMode(MODE_0_NORMAL);  // Mode 0 | normal operation

  //self-check initialization.
  WaitAUX_H();
  delay(10);
  
  if(STATUS == RET_SUCCESS)
    Serial.println("Setup init OK !!");
  else
    Serial.println("Setup init failed !!");
}


