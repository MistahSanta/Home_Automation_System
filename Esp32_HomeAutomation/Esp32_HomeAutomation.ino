// This is the main file that controls the motor and light sensor and sends this data off to the Connetion.h to handle sending the data to the Cloud 

#include <Connection.h>
#include <AccelStepper.h>
#include <ArduinoOTA.h>
#include <WiFi.h>


// For Window blinds motor pins 
#define ENABLE_PIN 4 // This pin will be use for 'sleeping' the stepper driver and stepper motor 
#define STEP_PIN 12
#define DIR_PIN  23

#define LIGHT_SENSOR_PIN 34 

#define STEPPER_TARGET_POSITION 1000
#define MOTOR_INTERFACE_TYPE 1
AccelStepper stepper = AccelStepper( MOTOR_INTERFACE_TYPE , STEP_PIN, DIR_PIN ); 
Connection* conn;

// Allow for more states in the future if needed 
bool System_State = 1; // MANUAL State = 0, AUTO = 1.  


bool motorMoving = false; // Track whether the light sensor updated from last value 
bool lastLightState = false;
int lightSensorValue = -1; 

void moveStepperToPosition( int new_position ) { 

    // First update stepper position
    stepper.moveTo(new_position);
    
    //enable stepper driver
    digitalWrite(ENABLE_PIN, LOW); 
    // Actually move the stepper motor 
    stepper.runToPosition(); // This blocks btw.
    
    // Turn OFF stepper driver to let it rest  
    digitalWrite( ENABLE_PIN, HIGH); 

}

void sendBrightnessInfo(int light_value, const char *topic) { 
    // Send this info to the DB 
    StaticJsonDocument<200> doc;
    doc["entity"] = "esp32"; 
    doc["Brightness"] = light_value > 3000 ? "Bright" : "Dark";
    doc["brightness_value"] = light_value;

    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);

    conn->publishMessage( jsonBuffer, topic);
}


// Callback function for onMessageRecieved for MQTT 
void onMessage( char *topic, byte* payload, unsigned int length ) { 
  StaticJsonDocument<512> doc; 

  DeserializationError error = deserializeJson(doc, payload);

  if ( error ) { 
    Serial.println("Failed to parse JSON!");
    Serial.println(error.f_str());
    return;
  }


  const char *entity = doc["entity"].as<const char*>();
  const char *req = doc["req"].as<const char*>();

  if ( strcmp(entity, "iphone") == 0 ) { 

    if ( strcmp(req, "GET") == 0 )  {
      // ! This part not tested yet
      const char *get_request = doc["light_sensor_value"];
      if ( strcmp(get_request, "true") == 0 ) {
        Serial.println("sending brightness value");
        sendBrightnessInfo(lightSensorValue, "iphone");
      } 
      return; 
    } else if ( strcmp(req, "SET") == 0) { 
      //FOR now, aassume set window blind state. 

      // Reach here, then we can parse 
      const char* mode = doc["System_State"]; 
      
      // Ensure correct mode is given - sanity check 
      if ( strcmp(mode, "AUTO") == 0 ) { 
        System_State = 1; // Set to AUTO mode 
        Serial.println("Setting Mode to Auto");
        return;
      }
    
      if ( strcmp(mode, "MANUAL") == 0 ) { 
        // Expected System_State and value to turn stepper Motor to 
        System_State = 0; // set to MANUAL mode 

        int stepper_new_position = doc["new_position"].as<int>();

        Serial.print("Moving to new position: ");
        Serial.println(stepper_new_position);

        // Now, we set the stepper motor 
        moveStepperToPosition( stepper_new_position );
        return; 
      }
  
    }
  }

  // Reach here, then error
  Serial.println("Unknown System State given! Ignoring...");

}




void setup() {
  Serial.begin(115200);
  delay(1000); // need this delay or else timing is mess up and Stepper Motor doesn't work!

  // Initialize everything for stepper motor and stepper driver 
  pinMode(ENABLE_PIN, OUTPUT); // DUHHH, can't believe I forgot to initialize this pin!
  pinMode(STEP_PIN, OUTPUT); // DUHHH, can't believe I forgot to initialize this pin!
  pinMode(DIR_PIN, OUTPUT); // DUHHH, can't believe I forgot to initialize this pin!
  
  digitalWrite( ENABLE_PIN, LOW); // Turn on stepper driver 
  stepper.setMaxSpeed(200.0);
  stepper.setAcceleration(25.0);
  // Move the motor to an initial position 
  stepper.setCurrentPosition(0);

  digitalWrite( ENABLE_PIN, HIGH); // Turn OFF stepper driver 

  //Initialize all the connection and MQTT protocol
  conn = new Connection();  
  // Handle connecting to MQTT and WIFI
  if ( !conn->connect_to_AWS() ) { 
    Serial.println("Unable to connect AWS IOT!");
  }

  //Subscribe to esp32/light_sensor_sub to get current state of Window Auto Opener 
  conn->subscribeTopic("esp32/light_sensor_sub");
  conn->setOnMessageCallback( onMessage );

}





// This function will check the photoresistor to ensure that it pass a certain threshold to determine if there is sunlight
// FIXME add a incremental way to check if sun is up 
// bool check_if_sun_up(){ 
  
// }

void check_move_blinds() { 
  lightSensorValue =  analogRead(LIGHT_SENSOR_PIN);
  bool currentLightState = ( lightSensorValue > 3000 );

  if ( currentLightState != lastLightState) {
    if ( currentLightState ) {
      // Assume sun is up, so move the motor 
      stepper.moveTo( STEPPER_TARGET_POSITION );

    } else if (lightSensorValue <= 3000 && !motorMoving ) {
      // Now just move to the original position
      stepper.moveTo(0);
    }

    // Set the motor flag to moving
    motorMoving = true;
    digitalWrite(ENABLE_PIN, LOW); //enable stepper driver
  }

  if ( motorMoving) {
    // Now, since we are moving, we should enable the stepper driver 

    stepper.runToPosition(); // This blocks btw.
    motorMoving = false; // reset the movign state when the target is reached
    digitalWrite( ENABLE_PIN, HIGH); // Turn OFF stepper driver 

    sendBrightnessInfo( lightSensorValue, "iphone" );
  }
  lastLightState = currentLightState;
}


void loop() {
  
  // DUHHH! This has to be called every now and then to maintain the connection! 
  conn->client.loop();
  ArduinoOTA.handle(); 
  
  //There is two state - MANUAL and AUTO. 
  if ( System_State ) { 
    // System is in Auto mode 
    check_move_blinds();
  } 

  


}
