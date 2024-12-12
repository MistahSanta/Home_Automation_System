// This file handles all the connection stuff between the ESP32 and AWS and WIFI
#ifndef CONNECTION_H
#define CONNECTION_H

#include <secrets.h>
#include <WiFi.h>
#include <PubSubClient.h> // This libraary is for MQTT, but doesn't seem to work with AWS 
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
//#include <MQTT.h>

class Connection {
public: 
  Connection();
    
  bool connect_to_wifi();
  bool connect_to_AWS(); 
  PubSubClient client;
  bool publishMessage( const char *payload, const char *message );
  void subscribeTopic( const char *topic );
  void setOnMessageCallback(std::function<void (char *, uint8_t *, unsigned int)> callback);

private:
  char* AWS_IOT_PUBLISH_TOPIC;
  char* AWS_IOT_SUB_TOPIC;
  WiFiClientSecure net;


  static void messageHandler(char* topic, byte* payload, unsigned int length); 
  
};




#endif