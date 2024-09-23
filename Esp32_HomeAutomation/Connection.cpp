#include <Arduino.h>
#include <Connection.h>

// Constructor 
Connection::Connection() {
  // Set up Network and MQTT parameters 
  net    = WiFiClientSecure();
  client = PubSubClient(net);
   
  // Modify these values to change the Endpoint for AWS IOT MQTT
  AWS_IOT_PUBLISH_TOPIC = "esp32/light_sensor_value";
  AWS_IOT_SUB_TOPIC     = "esp32/light_sensor_sub";
};

bool Connection::connect_to_AWS() { 
  WiFi.mode( WIFI_STA );
  WiFi.begin( WIFI_SSID, WIFI_PASSWORD );

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi!");
  
  // Verify that can actually access the internet
  IPAddress ip;
  if (WiFi.hostByName("www.google.com",  ip))
  {
    Serial.println("Internet access confirmed!");
  } else {
    Serial.println("No Internet access!");
  }



  // Configure WiFiClientSecure to use the AWS IoT device_credientials 
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  //Connect to the MQTT broker on the AWS endpoint
  client.setServer( AWS_IOT_ENDPOINT, 8883 );
  client.setCallback(Connection::messageHandler);

  Serial.println("Connecting to AWS IOT");

  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  //Subscribe to the topic
  Serial.print("AWS_SUB: ");
  Serial.println(AWS_IOT_SUB_TOPIC);
  if ( !client.subscribe( "esp32/light_sensor_sub", 0) ) {
    Serial.println("Failed to subscribe to topic!");
  }

  Serial.println("AWS IOT Connected!");

  return true;
};

// This function is used to send a message to an endpoint using a JSON format
bool Connection::publishMessage( char *payload ) 
{
  StaticJsonDocument<200> doc;
  doc["msg"] = payload;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  if ( client.publish( AWS_IOT_PUBLISH_TOPIC, jsonBuffer) ) {
    Serial.println("Message published successfully!");
    return true;
  } else {
    Serial.println("Message published failed");
    return false;
  }

}





// This function handles the message being sent to our endpoint 
void Connection::messageHandler( char* topic, byte* payload, unsigned int length )
{
  Serial.print("incoming: ");
  Serial.println(topic);

  StaticJsonDocument<200> doc;
  deserializeJson( doc, payload );
  const char* message = doc["message"];
  
  Serial.println(message);
}






