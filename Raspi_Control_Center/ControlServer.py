# This is a raspberry pi serving as a control server that communicates with an ESP32 and AWS 
# using MQTT protocol 


import time
import paho.mqtt.client as mqtt 
import ssl 
import json
from threading import Thread 

# This is a callback function
def on_connect( client, userdata, flags, reason_code, properties  ): 
    # This should be printed to show up that the connection to the MQTT was successful
    print('Connected to AWS IoT: ', str(reason_code), flush=True)

def on_publish( client, userdata, mid, reason_code, properties):
    print("messgae published!")


client = mqtt.Client( mqtt.CallbackAPIVersion.VERSION2 ) 
client.on_connect = on_connect 
client.on_publish = on_publish
client.tls_set( ca_certs="RootCA.pem", 
                certfile="certificate.pem.crt",
                keyfile ="private.key",
                tls_version=ssl.PROTOCOL_SSLv23,
               )
client.tls_insecure_set(True) # Only for testing, not for production

client.connect(host="a10cqv0k2kkn06-ats.iot.us-east-2.amazonaws.com", port=8883, keepalive=60 )


print("testing", flush=True)


client.loop_start()

ctr = 1
while( True ):
    msg = "Testing sending data using Raspberry pi! :" + str(ctr)
    print(msg)
    
    # Send the data!
    client.publish("raspi/data", payload=json.dumps({"msg": msg}), qos=0) 
    time.sleep(5)
    ctr += 1