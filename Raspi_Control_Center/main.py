# This is a raspberry pi serving as a control server that communicates with an ESP32 and AWS 
# using MQTT protocol 

import time, os, json
from threading import Thread 
from ImageDetection import capture_image, handle_3D_fail_detection
import paho.mqtt.client as mqtt 
from dotenv import load_dotenv

from MQTT import MQTT

load_dotenv() 

THING_NAME   = os.getenv("THING_NAME")
_TOPIC       = os.getenv("RASPI_ENDPOINT")
ESP32_TOPIC  = os.getenv("ESP32_ENDPOINT")
AWS_IOT      = os.getenv("AWS_IOT")



# This is a callback function for raaspberry Pi 
def raspi_on_connect( client, userdata, flags, reason_code, properties  ): 
    # This should be printed to show up that the connection to the MQTT was successful
    print(f"Subscriping to Topic1: {_TOPIC} and {ESP32_TOPIC}")
    client.subscribe(_TOPIC)  
    
def raspi_on_message( client: mqtt.Client, userdata, message: mqtt.MQTTMessage):
    response_endpoint = "iphone" 

    print(f"Recieved message to Pi: {message.payload.decode()}")
    
    try: 
        req = json.loads( message.payload.decode() )  
    except Exception as e: 
        print(f"Unexpected error when trying to deserialized JSON: {e}")
    
    # Request can either be a set or get command: 
    command_type = req['command_type']
    
    if command_type == "GET": 
        req_commands = req['commands']
        for r in req_commands: 
                
    elif command_type == "SET": 



def esp32_on_connect(  client, userdata, flags, reason_code, properties ):
    print(f"Subscriping to Topic1: {_TOPIC} and {ESP32_TOPIC}")
    client.subscribe( ESP32_TOPIC )  

def esp32_on_message(client: mqtt.Client, userdata, message: mqtt.MQTTMessage):
    """
    Handle message sent by esp32 
    """
    print(f"recieved message from ESP32: {message.payload.decode()}")



def handle_raspi_thread(): 
    # This thread 
    mqtt = MQTT() 

    client = mqtt.connectToMQTT(AWS_IOT_LINK=AWS_IOT, client_id="Raspi_Thread")
    
    # Must do this after connecting to MQTT 
    mqtt.setOnConnect( raspi_on_connect ) 
    mqtt.setOnMessage( raspi_on_message )
    
    client.loop_forever() 

def handle_esp32_thread(): 
    mqtt = MQTT()
    client = mqtt.connectToMQTT(AWS_IOT_LINK=AWS_IOT, client_id="ESP32_Thread")
    
    # Must do this after connecting to MQTT 
    mqtt.setOnConnect( esp32_on_connect ) 
    mqtt.setOnMessage( esp32_on_message )

    client.loop_forever()


def main(): 


    # Create a thread for each MQTT topic with its own client connection  
    RaspiThread = Thread(target=handle_raspi_thread)
    ESPThread   = Thread(target=handle_esp32_thread)
    
    RaspiThread.start()
    ESPThread.start()

    # just wait for a thread to end - will not happen since thread loop forever 
    RaspiThread.join()
    ESPThread.join()

    print("Program exited naturally")

    


if __name__ == "__main__": 
    main() 