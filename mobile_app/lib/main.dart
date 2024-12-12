import 'dart:convert';
import 'dart:io';
import 'dart:math';

import 'package:english_words/english_words.dart';
import 'package:flutter/material.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:provider/provider.dart';
import 'mqtt.dart'; 

void main() {
  runApp(MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return ChangeNotifierProvider(
      create: (context) => MyAppState(),
      child: MaterialApp(
        title: 'Home Automation App',
        theme: ThemeData(
          useMaterial3: true,
          colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurpleAccent),
        ),
        home: MyHomePage(),
      ),
    );
  }
}

class MyAppState extends ChangeNotifier {
  var current = WordPair.random();
  void getNext() { 
    current = WordPair.random(); 
    notifyListeners();
  }

  // Array 
  var favorities = <WordPair>[];

  void toggleFavorite() { 
    if( favorities.contains( current ) ) {
      favorities.remove(current);
    } else { 
      favorities.add(current);
    }
    notifyListeners();
  }

}


class BigCard extends StatelessWidget {
  const BigCard({
    super.key,
    required this.pair,
  });

  final WordPair pair;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);

    final style = theme.textTheme.displayMedium!.copyWith(
      color: theme.colorScheme.onPrimary,
    );

    return Card(
      color: theme.colorScheme.primary,
      child: Padding(
        padding: const EdgeInsets.all(20.0),
        child: Text( 
          pair.asLowerCase, 
          style:style,
          semanticsLabel: "${pair.first} ${pair.second}", 
          ),
      ),
    );
  }
}



class MyHomePage extends StatefulWidget { 
  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {

  int mode = 0; // Default to AUTO 
  String prediction = "Not running";
  String light_sensor_value = "-1"; 

  late MqttService mqttService = MqttService(); 
  final esp32_topic = "esp32/light_sensor_sub";
  final raspi_topic = "raspi/data";



  void _onMessage( MqttReceivedMessage message ) {
      final payload = message.payload as MqttPublishMessage;
      // Convert payload to string 
      final messageContent = MqttPublishPayload.bytesToStringAsString( payload.payload.message ); 

      //print("Recieved message ${messageContent}");
      
      // Decode json 
      try { 
        final decodedJson = jsonDecode(messageContent); 
        print("Decoded JSON: $decodedJson" );
        if ( decodedJson['entity'] == "raspi_control_center") { 
          setState(() {
            prediction = decodedJson['content']['prediction'];         

          });
        } else if ( decodedJson['entity'] == "esp32") {
          setState(() {
            light_sensor_value = decodedJson['brightness_value'].toString();
          });
        }
        
      } catch (e) { 
        print("Error when trying to decode JSON: $e");
      }
  }



  void _initializeMQTT() async { 
    await mqttService.connect();
    mqttService.subscribeToTopic("iphone");
    
    MqttClient? client = mqttService.getClient();
    // Handle message arriving to the subscribed Topic 
    client!.updates!.listen( (List<MqttReceivedMessage> messages) {
      // Process incoming message 
      
      for ( MqttReceivedMessage message in messages ) { 
        _onMessage(message);
      }




    });
    print("Finsihed initializing MQTT");
  }

  @override
  void initState() {
    super.initState();
    _initializeMQTT();
  }

  @override
  Widget build(BuildContext context) {

    Widget content; 
    switch( mode ) { 
      case 0: 
        content = Column(
          children: [
            Text("Brightness: ${(int.parse(light_sensor_value) < 3000) ? 'Dark' : 'Bright'}"),
            Text("Brightness level: $light_sensor_value"),
          ],
        );
        break;
      case 1: 
        content = Container ( 
          child: Column(
          children: [ Padding(
          padding: const EdgeInsets.all(8.0),
          child: ElevatedButton(onPressed: () async { // TODO fix me so that i am more dynamic 
            mqttService.publishMessage( '{"entity":"iphone", "req":"SET", "System_State": "MANUAL", "new_position": -500}', esp32_topic);
           }, child: Text("Close Blinds")),
         ),
         ElevatedButton(onPressed: () {  // TODO Fix me as well
            mqttService.publishMessage('{"entity":"iphone", "req":"SET", "System_State": "MANUAL", "new_position": 500}', esp32_topic);
            
         }, child: Text("Open Blinds")), 
      ]
            ));
        break;    
        default:
          content = Text("Not implemented yet!"); 
    }

    return LayoutBuilder( builder: (context, constraints) {
        return Scaffold( 
          body: Column(
            children: [


              // * This part is for the 3D printer stuff - ignore for now 
              Expanded(
                child: Container(
                  margin: const EdgeInsets.fromLTRB(30, 50, 30, 20),
                  color: Theme.of(context).colorScheme.primaryContainer,
                  child: Image( image:AssetImage('assets/images/random.jpg'), fit: BoxFit.fill) // ! issue with loading images dynamically 
              )),
              Container(
                child: Text("Print Prediction: Good"), // TODO change me to be dynamic using MQTT 
              ),
              Expanded(
                child: Container(
                  margin: const EdgeInsets.fromLTRB(30, 30, 30, 30),
                  color: Theme.of(context).colorScheme.primaryContainer,
                  child:  Column(
                    children: [
                      
                      Column(
                        children: [
                            Row( 
                              children: [
                                Expanded(
                                  child: Row(
                                    mainAxisAlignment: MainAxisAlignment.center,
                                    children: [
                                      Expanded( 
                                        child: TextButton(onPressed: () {
                                            mqttService.publishMessage( '{"entity":"iphone", "req":"GET", "light_sensor_value":"true"}' , esp32_topic);
                                            mqttService.publishMessage( '{"entity":"iphone", "req":"SET", "System_State": "AUTO"}', esp32_topic);

                                            setState(() {
                                              mode = 0;
                                            });
                                        }, child: Text("AUTO", ),
                                        style: TextButton.styleFrom(
                                          backgroundColor: mode == 0 ?  Colors.black12 : Colors.white10,
                                          shape: RoundedRectangleBorder(
                                            borderRadius: BorderRadius.zero,
                                          )
                                        ),
                                        )),
                                      Expanded( 
                                        child: TextButton( onPressed: () {
                                          setState(() {
                                            mode = 1;                                          
                                          });
                                        },child: Text("MANUAL"),
                                        style: TextButton.styleFrom(
                                          backgroundColor: mode == 1 ?  Colors.black12 : Colors.white10,
                                          shape: RoundedRectangleBorder(
                                            borderRadius: BorderRadius.zero,
                                          )
                                        ),
                                      ))
                                    ],),
                                )
                              ]
                              
                          ),
                          content  
                          
                        
                      
                        ],
                      ),

                    ],
                  )
                ),
              )
            ],
          )
        );
      }
    );
  }
}



class GeneratorPage extends StatelessWidget {

  final MqttService mqttService = MqttService(); 

  @override
  Widget build(BuildContext context) {
    var appState = context.watch<MyAppState>();
    var pair     = appState.current; 

    IconData icon; 
    // Switch icon state depending on the favorite list. 
    if( appState.favorities.contains(pair) ) { 
      icon = Icons.favorite;
    } else { 
      icon = Icons.favorite_border;
    }
    
    return Scaffold(
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center, // Center 
          children: [
            BigCard(pair: pair),
            SizedBox(height:20),
            Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                ElevatedButton.icon(onPressed: () {
                  appState.toggleFavorite();
                }, 
                icon: Icon(icon), 
                label: Text("Like") ),
                SizedBox(width: 20),
                ElevatedButton(onPressed: () { 
                  appState.getNext();
                }, child: Text("Next") ),
              ],
            )
          ],
          
        ),
      ),
    );
  }
}




// class FavoritePage extends StatelessWidget{

//   final MqttService mqttService = MqttService(); 
  
//   @override
//   Widget build(BuildContext context) {
//     var appState = context.watch<MyAppState>();
//     var list_fav = appState.favorities;

//     if ( list_fav.isEmpty ) { 
//       return Center(
//         child: Text("No favorite yet."),
//       );
//     }

//     return ListView(
//       children: [
//         Padding(padding: const EdgeInsets.all(20), 
//           child: Text('You have ' '${list_fav.length} favorites:'),
//         ),
//         for ( var pair in list_fav ) 
//         ListTile(
//           leading: Icon(Icons.favorite),
//           title: Text(pair.asLowerCase),
//         ),
//         ElevatedButton(onPressed: () async { 
//           await mqttService.connect(); 

//           mqttService.subscribeToTopic();

//           // Format into JSON 

//           // Publish 
//           mqttService.publishMessage('{"System_State": "MANUAL", "new_position": -500}');

//           await Future.delayed(Duration(seconds: 5));
//           mqttService.disconnect();

//         }, child: Text("Conencted to Mqtt Broker!")),
//       ],
//     );



//   }
// }

