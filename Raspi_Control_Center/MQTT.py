import paho.mqtt.client as mqtt 
import ssl


class MQTT: 
    
    def __init__(self):  
        self.__client: mqtt.Client= None 

    def connectToMQTT(self, AWS_IOT_LINK: str, client_id ) -> mqtt.Client: 
        """
        Connects to AWS and return a client. NOTE: Does not set callback functions 
        Use other function to set those callback function for MQTT 
        """
        client = mqtt.Client( callback_api_version=mqtt.CallbackAPIVersion.VERSION2, client_id=client_id, protocol=mqtt.MQTTv5) 
        client.tls_set( ca_certs="Certificates/RootCA.pem", 
                        certfile="Certificates/certificate.pem.crt",
                        keyfile ="Certificates/private.key",
                        tls_version=ssl.PROTOCOL_TLSv1_2,
                    )
        client.tls_insecure_set(True) # Only for testing, not for production

        print(AWS_IOT_LINK)
        print()
        print()

        client.connect(host=AWS_IOT_LINK, port=8883)
        self.__client = client 
        
        return client 
    

    
    def setOnMessage(self, callback_function):
        if self.__client == None: 
            print("Error! Pleasae call connectToMQTT to create a client first!")
            return 
        self.__client.on_message = callback_function
        
    def setOnPublish(self, callback_function):
        if self.__client == None: 
            print("Error! Pleasae call connectToMQTT to create a client first!")
            return 
        
        self.__client.on_publish = callback_function

        
    def setOnConnect(self, callback_function):
        if self.__client == None: 
            print("Error! Pleasae call connectToMQTT to create a client first!")
            return 
        
        self.__client.on_connect = callback_function
