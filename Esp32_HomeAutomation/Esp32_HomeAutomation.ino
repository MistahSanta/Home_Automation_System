// This is the main file that controls the motor and light sensor and sends this data off to the Connetion.h to handle sending the data to the Cloud 

#include <Connection.h>
#include <AccelStepper.h>
#include <ArduinoOTA.h>
#include <WiFi.h>


// For Window blinds motor pins 
#define ENABLE_PIN 4 // This pin will be use for 'sleeping' the stepper driver and stepper motor 
#define STEP_PIN 12
#define DIR_PIN  23

// For Fan Control Pins 
const int FAN_LIGHT_PIN      = 13;
const int FAN_ON_OFF_PIN     = 15;
const int FAN_SPEED_HIGH_PIN = 2;
const int FAN_SPEED_MED_PIN  = 0;
const int FAN_SPEED_LOW_PIN  = 16; 

// For Bed control system
const int BED_HEAD_UP_POSITION_PIN = 27;
const int BED_HEAD_DOWN_POSITION_PIN = 26;
const int BED_FOOT_UP_POSITION_PIN = 25;
const int BED_FOOT_DOWN_POSITION_PIN = 33;


#define LIGHT_SENSOR_PIN 34 

#define STEPPER_TARGET_POSITION 1000
#define MOTOR_INTERFACE_TYPE 1
AccelStepper stepper = AccelStepper( MOTOR_INTERFACE_TYPE , STEP_PIN, DIR_PIN ); 
Connection* conn;

// Allow for more states in the future if needed 
bool System_State = 0; // MANUAL State = 0, AUTO = 1.  
int isWaiting[] = {false, 0, 0}; // 1st element = is it waiting, 2nd element the button pin it is waiting, 3rd element = delay amount  
unsigned long int  time_since_button_press = 0;  


bool motorMoving = false; // Track whether the light sensor updated from last value 
bool lastLightState = false;
int lightSensorValue = -1; 
int seconds_since_move_blinds = millis() + 9999999;

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

void holdButtonPress( int pin, int time_ms ) { 

  // Handle race conditiosn 
  if (isWaiting[0] == true) { 
    // There is already a button that is being press so ignore this one 
    Serial.println("There is already a button being pressed!");
    return; 
  }
  digitalWrite(pin, LOW );
  isWaiting[0] = true;
  isWaiting[1] = pin; 
  isWaiting[2] = time_ms;
  time_since_button_press = millis();
}

// Callback function for onMessageRecieved for MQTT 
void onMessage( char *topic, byte* payload, unsigned int length ) { 

  StaticJsonDocument<1024> doc; 

  DeserializationError error = deserializeJson(doc, payload);

  if ( error ) { 
    Serial.println("Failed to parse JSON!");
    Serial.println(error.f_str());
    return;
  }


    const char *entity = doc["entity"].as<const char*>();
  const char *req = doc["req"].as<const char*>();

  if ( strcmp(entity, "iphone") == 0 ) { 

    // Check if there is a reset key word 
      if ( doc["reset"].is<int>() ) { // Set new home position 
          stepper.setCurrentPosition(0);
          return;
      }


    if ( strcmp(req, "GET") == 0 )  {
      // ! This part not tested yet
      const char *get_request = doc["light_sensor_value"];
      if ( strcmp(get_request, "true") == 0 ) {
        Serial.println("sending brightness value");
        sendBrightnessInfo(lightSensorValue, "iphone");
      } 
      return; 
    } else if ( strcmp(req, "SET") == 0) { 

      // Check if we need to handle Fan control 
        if ( doc["Fan_System"].is<const char *>() ) { // Handle fan state  
          const char *fan_command = doc["Fan_System"]; 
          Serial.print("Fan command given: ");
          Serial.println(fan_command);

          if ( strcmp( fan_command, "fan_light") == 0  ) { 
            press_fan_button(FAN_LIGHT_PIN);
          } else if (strcmp( fan_command, "fan_speed_low") == 0) {
            press_fan_button(FAN_SPEED_LOW_PIN);
          } else if ( strcmp( fan_command, "fan_speed_med" ) == 0 ) { 
            press_fan_button(FAN_SPEED_MED_PIN);
          } else if ( strcmp( fan_command, "fan_speed_high" ) == 0 ) { 
            press_fan_button(FAN_SPEED_HIGH_PIN);
          }  else if ( strcmp( fan_command, "fan_speed_on_off" ) == 0 ) { 
            press_fan_button(FAN_ON_OFF_PIN);
          } else {
            Serial.println("Unknown Fan state command given!");
          }

          Serial.println("Handled Fan_System command!");
          return;
      }

      if ( doc["Bed_move_up_head_position"].is<int>() ) { // Handle fan state  
          int time_hold_button = doc["Bed_move_up_head_position"]; 
          Serial.print("Bed up head command given: ");

   
   
          holdButtonPress( BED_HEAD_UP_POSITION_PIN, time_hold_button);     
          return; 
      }

      if ( doc["Bed_move_down_head_position"].is<int>() ) { // Handle fan state  
          int time_hold_button = doc["Bed_move_down_head_position"]; 
          Serial.print("Bed up head command given: ");

   
   
          holdButtonPress( BED_HEAD_DOWN_POSITION_PIN, time_hold_button);     
          return; 
      }
      if ( doc["Bed_move_up_feet_position"].is<int>() ) { // Handle fan state  
          int time_hold_button = doc["Bed_move_up_feet_position"]; 
          Serial.print("Bed up feet command given: ");

   

          holdButtonPress( BED_FOOT_UP_POSITION_PIN, time_hold_button);     
          return; 
      }
      if ( doc["Bed_move_down_feet_position"].is<int>() ) { // Handle fan state  
          int time_hold_button = doc["Bed_move_down_feet_position"]; 
          Serial.print("Bed down feet command given: ");

   
   
          holdButtonPress( BED_FOOT_DOWN_POSITION_PIN, time_hold_button);     
          return; 
      }

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
// Function that activates the fan pin like pressing a button on the remote 
void press_fan_button( int fan_pin ) {

  digitalWrite(fan_pin, LOW );
  delay(200);
  digitalWrite( fan_pin, HIGH );
}



void setup() {
  Serial.begin(115200);
  delay(1000); // need this delay or else timing is mess up and Stepper Motor doesn't work!

  // Initialize everything for stepper motor and stepper driver 
  pinMode(ENABLE_PIN, OUTPUT); // DUHHH, can't believe I forgot to initialize this pin!
  pinMode(STEP_PIN, OUTPUT); // DUHHH, can't believe I forgot to initialize this pin!
  pinMode(DIR_PIN, OUTPUT); // DUHHH, can't believe I forgot to initialize this pin!
  pinMode( FAN_LIGHT_PIN, OUTPUT );
  pinMode( FAN_ON_OFF_PIN, OUTPUT );
  pinMode( FAN_SPEED_HIGH_PIN, OUTPUT );
  pinMode( FAN_SPEED_MED_PIN, OUTPUT );
  pinMode( FAN_SPEED_LOW_PIN, OUTPUT );


  pinMode(BED_HEAD_UP_POSITION_PIN, OUTPUT);
  pinMode(BED_HEAD_DOWN_POSITION_PIN, OUTPUT);
  pinMode(BED_FOOT_UP_POSITION_PIN, OUTPUT);
  pinMode(BED_FOOT_DOWN_POSITION_PIN, OUTPUT);

  // Initialize FAN state to all off ( HIGH ) - since fan pins are active HIGH 
  digitalWrite( FAN_LIGHT_PIN, HIGH );
  digitalWrite( FAN_ON_OFF_PIN, HIGH );
  digitalWrite( FAN_SPEED_HIGH_PIN, HIGH );
  digitalWrite( FAN_SPEED_MED_PIN, HIGH );
  digitalWrite( FAN_SPEED_LOW_PIN, HIGH );

  digitalWrite( BED_HEAD_UP_POSITION_PIN, HIGH );
  digitalWrite( BED_HEAD_DOWN_POSITION_PIN, HIGH );
  digitalWrite( BED_FOOT_UP_POSITION_PIN, HIGH );
  digitalWrite( BED_FOOT_DOWN_POSITION_PIN, HIGH );








  digitalWrite( ENABLE_PIN, LOW); // Turn on stepper driver 
  stepper.setMaxSpeed(250.0);
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

  //conn->publishMessage( "HELLO FROM ESP32 using TLS", "topic/test");

}





// This function will check the photoresistor to ensure that it pass a certain threshold to determine if there is sunlight
// FIXME add a incremental way to check if sun is up 
// bool check_if_sun_up(){ 
  
// }

// This function handles opening and closing the blinds
void handle_auto_mode( ) { 
  // Since I am over currenting my stepper motor, i want to give it enough time to rest, so 

  // we only open or close the blinds every 10 minutes 
  if ( millis() -  seconds_since_move_blinds < 600000 ) return; 

  lightSensorValue =  analogRead(LIGHT_SENSOR_PIN);
  bool currentLightState = ( lightSensorValue > 3500 );

  if ( currentLightState != lastLightState) {
    // Just for responsivenesss, we will send the message to iPhone then update motor position
    sendBrightnessInfo( lightSensorValue, "iphone" );
    
    if ( currentLightState ) {
      // Assume sun is up, so move the blinds to the open position 
      moveStepperToPosition( STEPPER_TARGET_POSITION );
    } else {
      // Now just move to the original or closed position
      moveStepperToPosition( 0 );
    }
   seconds_since_move_blinds = millis(); 
  }
  lastLightState = currentLightState;

}


void loop() {
  // Wifi and MQTT might disconnect, so if either of those happen, we just simply reconnect 
  if (WiFi.status() != WL_CONNECTED) { 
    Serial.println("Wifi is not connected. Reconnecting");
    Serial.println(WiFi.status());
    if ( ! conn->reconnect_to_wifi() ) { 
      // failed to reconnect so just restart the ESP32
      Serial.println("Failed to reconnect to WiFi! Restarting the ESP32!");
      ESP.restart();
    }
  }
  if ( !conn->client.connected() ) {
    if ( !conn->reconnect_to_mqtt() ) {
      Serial.println("Failed to reconnect to MQTT broker! Restarting the ESP32!");
      ESP.restart(); 
    }
  }

  // DUHHH! This has to be called every now and then to maintain the connection! 
  conn->client.loop();
  ArduinoOTA.handle(); 
  

  //There is two state - MANUAL and AUTO. 
  if ( System_State ) { 
    // System is in Auto mode 
    handle_auto_mode();
  } 

  // I hate the way I have to do this tbh since the only way 
  // to have a nonblocking delay is to use a flag,
  if ( isWaiting[0] == true ) {
    int button_pin    = isWaiting[1];
    int delay_time_ms = isWaiting[2]; 

    if ( millis() - time_since_button_press >= delay_time_ms ) {
      // Stop pressing the button 
      digitalWrite( button_pin, HIGH);
      isWaiting[0] = false; 
      isWaiting[1] = 0;
    }
  }

  


}
