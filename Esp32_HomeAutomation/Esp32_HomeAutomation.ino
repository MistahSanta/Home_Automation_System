// This is the main file that controls the motor and light sensor and sends this data off to the Connetion.h to handle sending the data to the Cloud 

#include <Connection.h>
#include <AccelStepper.h>

// For Window blinds motor pins 
#define ENABLE_PIN 4 // This pin will be use for 'sleeping' the stepper driver and stepper motor 
#define STEP_PIN 22
#define DIR_PIN  23

#define LIGHT_SENSOR_PIN 34 

#define STEPPER_TARGET_POSITION 1000
#define MOTOR_INTERFACE_TYPE 1
AccelStepper stepper = AccelStepper( MOTOR_INTERFACE_TYPE , STEP_PIN, DIR_PIN ); 
Connection* conn;

bool motorMoving = false; // Track whether the light sensor updated from last value 
bool lastLightState = false;

void setup() {
  Serial.begin(115200);
  delay(1000); // need this delay or else timing is mess up and Stepper Motor doesn't work!
  
  // Initialize everything for stepper motor and stepper driver 
  pinMode(ENABLE_PIN, OUTPUT); // DUHHH, can't believe I forgot to initialize this pin!
  digitalWrite( ENABLE_PIN, LOW); // Turn on stepper driver 
  stepper.setMaxSpeed(500.0);
  stepper.setAcceleration(200.0);
  // Move the motor to an initial position 
  stepper.setCurrentPosition(0);



  digitalWrite( ENABLE_PIN, HIGH); // Turn OFF stepper driver 

  //Initialize all the connection and MQTT protocol
  conn = new Connection();  
  // Handle connecting to MQTT and WIFI
  if ( !conn->connect_to_AWS() ) { 
    Serial.println("Unable to connect AWS IOT!");
  }


}



// This function will check the photoresistor to ensure that it pass a certain threshold to determine if there is sunlight
// FIXME add a incremental way to check if sun is up 
// bool check_if_sun_up(){ 
  
// }

void check_move_blinds() { 
  int light_value =  analogRead(LIGHT_SENSOR_PIN);
  bool currentLightState = ( light_value > 3000 );
  char* light_state_name = "Dark";

  if ( currentLightState != lastLightState) {
    if ( currentLightState ) {
      // Assume sun is up, so move the motor 
      stepper.moveTo( STEPPER_TARGET_POSITION );
      light_state_name = "Dark";
    } else if (light_value <= 3000 && !motorMoving ) {
      // Now just move to the original position
      stepper.moveTo(0);
      light_state_name = "Bright";
    }

    // Set the motor flag to moving
    motorMoving = true;
    digitalWrite(ENABLE_PIN, LOW); //enable stepper driver
  }

  if ( motorMoving) {
    // Now, since we are moving, we should enable the stepper driver 

    stepper.runToPosition(); // This blocks btw.
    Serial.println("HUH!");
    motorMoving = false; // reset the movign state when the target is reached
    digitalWrite( ENABLE_PIN, HIGH); // Turn OFF stepper driver 

    // Send this info to the DB 
    StaticJsonDocument<200> doc;
    doc["sensor_id"] = 1; 
    doc["Brightness"] = light_state_name;
    doc["brightness_level"] = light_value;

    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);

    conn->publishMessage( jsonBuffer );

  }

  lastLightState = currentLightState;
}

void loop() {
  
  // DUHHH! This has to be called every now and then to maintain the connection! 
  conn->client.loop();

  check_move_blinds();



  // delay(5000);

}
