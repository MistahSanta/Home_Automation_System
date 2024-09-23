// This is the main file that controls the motor and light sensor and sends this data off to the Connetion.h to handle sending the data to the Cloud 

#include <Connection.h>
#include <AccelStepper.h>

// For Window blinds motor pins 
#define ENABLE_PIN 21 // This pin will be use for 'sleeping' the stepper driver and stepper motor 
#define STEP_PIN 22
#define DIR_PIN  23

#define LIGHT_SENSOR_PIN 34 





//AccelStepper stepper = AccelStepper( AccelStepper::DRIVER, STEP_PIN, DIR_PIN ); 
Connection* conn;

void setup() {
  Serial.begin(115200);
  // digitalWrite( ENABLE_PIN, LOW); // Enable the stepper driver   
  // stepper.setMaxSpeed(2000);
  // stepper.setAcceleration(1000);


  // // Move the motor to an initial position 
  // stepper.setCurrentPosition(0);
  // stepper.moveTo(1000);
  conn = new Connection();
  
  // Handle connecting to MQTT and WIFI
  if ( ! conn->connect_to_AWS() ) { 
    Serial.println("Unable to connect AWS IOT!");
  }


}

void loop() {
  
  // DUHHH! This has to be called every now and then to maintain the connection! 
  conn->client.loop();

  int light_value =  analogRead(LIGHT_SENSOR_PIN);

  char msg[50];
  sprintf(msg, "The light value is: %d", light_value);
  conn->publishMessage( msg );

  delay(5000);

  // stepper.run();

  // if ( stepper.distanceToGo() == 0 ) {
  //   stepper.moveTo( -stepper.currentPosition() );
  // }
}
