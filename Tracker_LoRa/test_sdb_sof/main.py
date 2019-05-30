from network import LoRa
import socket
import time



modes=[LoRa.LORA]
coding=[LoRa.CODING_4_5,LoRa.CODING_4_6,LoRa.CODING_4_7,LoRa.CODING_4_8]
pub=[True,False]

for k in modes:
    for i in range(0,16):
        for j in range(0,6):
            for l in coding:
                for p in pub:
                    #LoRa
                    # Europe = LoRa.EU868
                    #print("TEST1")
                    lora = LoRa(mode=k, region=LoRa.EU868)
                    #print("TEST2")
                    lora.init(frequency=885000000, mode=k, tx_power=14, sf=7+int(j),preamble=int(i),coding_rate=l,public=p)
                    #print("TEST3")
                    s = socket.socket(socket.AF_LORA, socket.SOCK_RAW)
                    #print("TEST4")
                    s.setblocking(False)
                    #print("LoRa initialise !")

                    flag= True
                    compteur = 0
                    print('Debut Test : ', 'sf =', 7+int(j), ', mode =',  k ,', preamble = ', i, ', coding_rate =', l)

                    while flag:

                        data = s.recv(128)

                        if data != b'':
                            print (data)


                        time.sleep(0.1)
                        compteur+=1

                        if compteur >= 10:
                            flag=False

                    s.close()

print('Fin Test\n')
