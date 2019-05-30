from network import LoRa
from mqtt import MQTTClient
from network import WLAN
import socket
import time
import math

lng = 0
lat = 0
posUInitialized = False
research=False

#LoRa
# Europe = LoRa.EU868
lora = LoRa(mode=LoRa.LORA, region=LoRa.EU868)
lora.init(mode=LoRa.LORA, frequency=868850000, tx_power=14, bandwidth=LoRa.BW_125KHZ, sf=12, preamble=4, coding_rate=LoRa.CODING_4_5, public=False)
s = socket.socket(socket.AF_LORA, socket.SOCK_RAW)
s.setblocking(False)
print("LoRa initialise !")


#WiFi
wlan = WLAN(mode=WLAN.STA)
wlan.connect(ssid='AndroidAPpir', auth=(WLAN.WPA2, 'bob31100'))
while not wlan.isconnected():
    time.sleep_ms(50)
print(wlan.ifconfig())
print("WiFi connectee !")


# Received messages from subscriptions will be delivered to this callback
def sub_cb(topic, msg):
    global lng
    global lat
    global posUInitialized
    global research

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
    elif topic[0:26] == b"v1/devices/me/rpc/request/" :
        for k in range(len(msg)):
            if msg[k]== ord("p"):
                if msg[k+1] == ord("a"):
                    if msg[k+2] == ord("r"):
                        if msg[k+8] == ord("f"):
                            research = False
                        else:
                            research=True
    else:
        posUInitialized= True
        msg_1=0
        msg_2=0
        i0=0
        for k in range(len(msg)):
            if msg[k]==ord("a"):
                i0=0
                negatif = False
                if msg[k+1]==ord("t"):
                    if msg[k+6]==ord("-"):
                        N0=k+7
                        j=0
                        negatif = True
                    else:
                        N0=k+6
                        j=0
                        negatif = False
                    while msg[N0+j]!=ord(","):
                        if msg[N0+j]!=ord("."):
                            msg_1=10*msg_1+(msg[N0+j]-48)
                            j+=1
                        else:
                            i0=j
                            j+=1
                    puiss=j-i0
                    lat=msg_1*10**(-(j-i0-1))
                    if negatif:
                        lat=lat*(-1)
                    print("lat",lat)
            if msg[k]==ord("n"):
                i0=0
                negatif = False
                if msg[k+1]==ord("g"):
                    if msg[k+6]==ord("-"):
                        N0=k+7
                        j=0
                        negatif = True
                    else:
                        N0=k+6
                        j=0
                        negatif = False
                    while msg[N0+j]!=ord("}"):
                        if msg[N0+j]!=ord("."):
                            msg_2=10*msg_2+(msg[N0+j]-48)
                            j+=1
                        else:
                            i0=j
                            j+=1
                    puiss=j-i0
                    lng=msg_2*10**(-(j-i0-1))
                    if negatif:
                        lng=lng*(-1)
                    print("lng",lng)





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
mqtt.subscribe(b"v1/devices/me/rpc/request/+")
mqtt.publish(b"v1/devices/me/attributes","{\"research\":false}")
# "{\"id\": 1, \"device\": \"Balise_Cat\", \"client\": True, \"key\": \"lng_u\"}"
#mqtt.publish(b"v1/devices/me/attributes/request","{\"id\": 1, \"device\": \"Balise_Cat\", \"client\": True, \"key\": \"lat_u\"}")
while(not posUInitialized):
    mqtt.check_msg()

print("sorti de la boucle")
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

# Constantes de comparaison

bat_min = 25
dist_faible=0.20
dist_loin=0.65
v_faible=2.78
v_haute=8.33

distance = 0
batterie=100
T_wait=5
distance_prec=distance

while True:

    mqtt.check_msg()
    lat1=lat
    long1=lng
    print("lat1 : ",lat1,"long1 : ",long1)
    print("research : ", research)

    data = s.recv(10)
    print("data : ",data)
    if len(data)>0:
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

            #Calcul de la distance device/sdb
            a=(((math.radians(long1)-math.radians(long2))/2)**2)
            distance_prec=distance
            distance=6366*2*math.asin(math.sqrt((math.sin(((math.radians(lat1)-math.radians(lat2))/2)))**2 + math.cos(math.radians(lat1))*math.cos(math.radians(lat2))*(math.sin(a))))
            print(distance,"km")
            mqtt.publish(TOPIC,"{\"latitude\":" + str(lat2) + ",\"longitude\":" + str(long2) + ",\"power\":" + str(lora.stats()[1]) + ",\"distance\":" + str(distance) + ",\"batterie\":" + str(batterie) + "}")

        #Gestion de la batterie
        T_wait_prec=T_wait
        T_wait = 30
        if research :
            if batterie > bat_min:
                T_wait = 5
            else:
                T_wait = 30
        else:
            v_act=abs(distance-distance_prec)*1000/T_wait_prec
            if batterie>bat_min:
                if distance>dist_loin:
                    if v_act>v_haute:
                        T_wait=10
                    elif v_act<v_faible:
                        T_wait=60
                    else:
                        T_wait=30
                elif distance<dist_faible:
                    if v_act>v_haute:
                        T_wait=30
                    elif v_act<v_faible:
                        T_wait=180
                    else:
                        T_wait=90
                else:
                    if v_act>v_haute:
                        T_wait=20
                    elif v_act<v_faible:
                        T_wait=120
                    else:
                        T_wait=60
            else:
                if distance>dist_loin:
                    T_wait=360
                elif distance<dist_faible:
                    T_wait=1080
                else:
                    T_wait=720
    print("T_wait",T_wait)
    T_wait_To_send=int((T_wait*255)/1080)
    if len(data)>0:
        s.setblocking(True)
        s.send(chr(T_wait_To_send))
        s.setblocking(False)
    time.sleep(1)

mqtt.disconnect()

