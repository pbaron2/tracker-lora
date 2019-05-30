from network import LoRa
from mqtt import MQTTClient
from network import WLAN
import socket
import time
import math

lng = 0
lat = 0
posUInitialized = False

#LoRa
# Europe = LoRa.EU868
lora = LoRa(mode=LoRa.LORA, region=LoRa.EU868)
lora.init(mode=LoRa.LORA, frequency=868850000, tx_power=14, bandwidth=LoRa.BW_125KHZ, sf=12, preamble=4, coding_rate=LoRa.CODING_4_5, public=False)
s = socket.socket(socket.AF_LORA, socket.SOCK_RAW)
s.setblocking(True)
print("LoRa initialise !")


#WiFi
wlan = WLAN(mode=WLAN.STA)
wlan.connect(ssid='AndroidAP', auth=(WLAN.WPA2, 'otty7433'))
while not wlan.isconnected():
    time.sleep_ms(50)
print(wlan.ifconfig())
print("WiFi connectee !")

# Received messages from subscriptions will be delivered to this callback
def sub_cb(topic, msg):
    global lng
    global lat
    global posUInitialized
    print("le topic est:",topic)
    print("msg : ", msg)
    if topic == b"v1/devices/me/attributes" :
        if msg[9]==ord("-"):
            N0 = 10
        else:
            N0 = 9
        N1=len(msg)-1
        msg_1=0
        i0=-1
        for i in range(N0,N1):
            if msg[i]!=ord("."):
                msg_1=10*msg_1+(msg[i]-48)
            else:
                i0=(N1-1)-i
        if N0==10:
            msg_1=msg_1*(-1)
        if i0!=-1:
            msg_1=msg_1*10**(-i0)
        if msg[3]==97:
            lat = msg_1
        else:
            lng = msg_1
    else:
        posUInitialized = True
        N1=len(msg)-1
        #print(msg[38],ord("-"),"19")
        if msg[19]==ord("-"):
            N0 = 20
        else:
            N0 = 19
        msg_1=0
        i0=-1
        for i in range(N0,N0+9):
            if msg[i]!=ord("."):
                #print(msg[i])
                msg_1=10*msg_1+(msg[i]-48)
            else:
                i0=(N1-22)-i
                #print(i0,"point")
        if N0==20:
            msg_1=msg_1*(-1)
        if i0!=-1:
            msg_1=msg_1*10**(-i0)
        lat = msg_1

        if N0== 19:
            if msg[38]==ord("-"):
                N0 = 39
            else:
                N0 = 38
        else:
            if msg[39]==ord("-"):
                N0 = 40
            else:
                N0 = 39

        msg_2=0
        i0=-1
        for i in range(N0,N0+9):
            if msg[i]!=ord("."):
                print(msg[i])
                msg_2=10*msg_2+(msg[i]-48)
            else:
                i0=(N1-2)-i
                #print(i0,"point")
        if N0==39:
            msg_2=msg_2*(-1)
        if i0!=-1:
            msg_2=msg_2*10**(-i0)
        lng = msg_2
        print("lat et long",msg_1,msg_2)






# MQTT
SERVER = b"ec2-18-222-208-169.us-east-2.compute.amazonaws.com"
TOKEN = b"iZ9RYl7KdOUpotHTSgSj"
CLIENT_ID = b"1d7834b0-d8fe-11e8-932c-2b02d679dd31"
TOPIC = b"v1/devices/me/telemetry"
mqtt = MQTTClient(CLIENT_ID, SERVER, user = TOKEN, password = "")
mqtt.set_callback(sub_cb)
mqtt.connect()
print("MQTT connectee !")
mqtt.subscribe(b"v1/devices/me/attributes")

mqtt.subscribe(b"v1/devices/me/attributes/responses/+")
mqtt.publish(b"v1/devices/me/attributes/request/1","{\"sharedKeys\":\"lat_u,lng_u\"}")
# "{\"id\": 1, \"device\": \"Balise_Cat\", \"client\": True, \"key\": \"lng_u\"}"
#mqtt.publish(b"v1/devices/me/attributes/request","{\"id\": 1, \"device\": \"Balise_Cat\", \"client\": True, \"key\": \"lat_u\"}")

while(not posUInitialized):
    print("test")
    mqtt.check_msg()

lat1=0
long1=0

def bytes_to_int(bytes):
    result = 0
    for by in bytes:
        by_0=int(by)
        result = result * 256 + by_0
        if result>(2**(31)-1):
            result=-1*(2**(32)-result)
    return result

def verif(data):
    N=len(data)
    tst=int(data[N-1])
    tst_0=0
    for k in range(0,N-1):
        tst_0=tst_0+int(data[k])
    a = False
    tst_0=tst_0%256
    if tst_0==tst:
        a=True
    return(a)

distance = 0

while True:

    mqtt.check_msg()
    print(lat)
    lat1=lat
    long1=lng
    print("lat1 : ",lat1,"long1 : ",long1)

    data = s.recv(10)
    print("data : ",data)
    check=verif(data)
    print("verif : ",check)
    if check == True:
        lat2 = bytes_to_int(data[0:4])/10000000
        print("lat2 : ",lat2)
        long2 = bytes_to_int(data[4:8])/10000000
        print("long2 : ",long2)

        batterie = int(data[8])
        print("batt : ",batterie)

        print("Puissance reçue en dBm : ",lora.stats()[1])

        distance = 1852*60*math.degrees(math.acos((math.sin(math.radians(lat1))*math.sin(math.radians(lat2))+math.cos(math.radians(lat1))*math.cos(math.radians(lat2))*math.cos(math.radians(long2-long1)))))
        print("distance en m : ",distance)

        mqtt.publish(TOPIC,"{\"latitude\":" + str(lat2) + ",\"longitude\":" + str(long2) + ",\"power\":" + str(lora.stats()[1]) + ",\"distance\":" + str(distance) + ",\"batterie\":" + str(batterie) + "}")

    time.sleep(1)

mqtt.disconnect()
