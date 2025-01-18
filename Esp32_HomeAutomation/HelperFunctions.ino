
// // This function handles opening and closing the blinds
// void handle_auto_mode( int seconds_since_move_blinds ) { 
//   // Since I am over currenting my stepper motor, i want to give it enough time to rest, so 

//   // we only open or close the blinds every 10 minutes 
//   if ( millis() -  seconds_since_move_blinds < 600000 ) return; 

//   lightSensorValue =  analogRead(LIGHT_SENSOR_PIN);
//   bool currentLightState = ( lightSensorValue > 3500 );

//   if ( currentLightState != lastLightState) {
//     // Just for responsivenesss, we will send the message to iPhone then update motor position
//     conn->sendBrightnessInfo( lightSensorValue, "iphone" );
    
//     if ( currentLightState ) {
//       // Assume sun is up, so move the blinds to the open position 
//       moveStepperToPosition( STEPPER_TARGET_POSITION );
//     } else {
//       // Now just move to the original or closed position
//       moveStepperToPosition( 0 );
//     }
//    seconds_since_move_blinds = millis(); 
//   }
//   lastLightState = currentLightState;

// }
// void moveStepperToPosition( int new_position ) { 
//     // First update stepper position
//     stepper.moveTo(new_position);
    
//     //enable stepper driver
//     digitalWrite(ENABLE_PIN, LOW); 
//     // Actually move the stepper motor 
//     stepper.runToPosition(); // This blocks btw.
    
//     // Turn OFF stepper driver to let it rest  
//     digitalWrite( ENABLE_PIN, HIGH); 
// }