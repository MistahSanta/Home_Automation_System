// MQTT Stuff 

import 'dart:typed_data';

import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import 'dart:io';
import 'package:flutter/services.dart' show rootBundle;
import 'dart:convert';

class MqttService { 

  MqttClient? _client;


  Future<Uint8List> _loadCertificate( String filename ) async {  
      final ByteData data = await rootBundle.load(filename);
      return data.buffer.asUint8List();
  }

  //Set up Mqtt Client and connect to AWS IOT Core broker 
  Future<void> connect() async  { 
    const url          = "a10cqv0k2kkn06-ats.iot.us-east-1.amazonaws.com";
    const port         = 8883;
    const clientId     = "iPhone";
    final ca_cert     = await _loadCertificate( "assets/Certificates/IphoneRootCA1.pem" );
    final private_key = await _loadCertificate("assets/Certificates/iphone_private.key");
    final device_cert = await _loadCertificate("assets/Certificates/iphone_device.crt");
    
    // Create the client - TCP 
    final client = MqttServerClient(url, clientId);

    client.secure = true; 
    client.port = port;
    client.keepAlivePeriod = 60; 
    client.setProtocolV311(); // Set MQTT protocol to 3.1.1 for AWS IOT core 
    client.logging(on: false);
    client.connectTimeoutPeriod = 10000;
    final context = SecurityContext.defaultContext;
    
    context.setTrustedCertificatesBytes( ca_cert ); 
    context.useCertificateChainBytes( device_cert );
    context.usePrivateKeyBytes( private_key );
    client.securityContext = context; 


    // Setup the connection Message 
    final connMess = MqttConnectMessage().withClientIdentifier( clientId ).startClean();

    client.connectionMessage = connMess; 
    
    // Try to connect the client 
    try { 
      print("MQTT client connecting to AWS using certs...");
      await client.connect();
    } on Exception catch (e) { 
      print('MQTT client Error when connecting: $e');
      client.disconnect();
      return; 
    }

    if( client.connectionStatus!.state != MqttConnectionState.connected) {
      print("Unable to connect to AWS broker! Conenction state: ${client.connectionStatus!.state}");
      return; 
    }

    print("Connected! ");



    _client = client; 
    



  }



  void subscribeToTopic(String topic) {

    if(_client == null ) {
      print("Client is not defined yet! Use mqttsetup() before using this function!");
      return; 
    }

    _client!.subscribe( topic, MqttQos.atLeastOnce);

    
  }
  
  void publishMessage(String msg, String topic  ) { 
    if(_client == null ) {
      print("Client is not defined yet! Use mqttsetup() before using this function!");
      return; 
    }

    print("Publishing msg: $msg");
    final builder = MqttClientPayloadBuilder();
    builder.addString( msg ); 

    _client!.publishMessage( topic, MqttQos.atMostOnce,  builder.payload! );
  }

  void disconnect() { 
    print("Disconnecting fom AWS broker" ); 
    _client?.disconnect();
  }

  MqttClient? getClient() { 
    return _client; 
  }
}