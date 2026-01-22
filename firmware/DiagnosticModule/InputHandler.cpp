#include <Arduino.h>
#include "InputHandler.h"
#include "config.h"


int distance = 0; 
long duration = 0;
int DIST_THRESHOLD = 60; //60cm away triggers screen and led speed


void init_inputs(){
  //configure LED pin
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);


  //configure joystick pins
  pinMode(PIN_JOYSTICK_X, INPUT);
  pinMode(PIN_JOYSTICK_Y, INPUT);
  pinMode(PIN_JOYSTICK_SW, INPUT_PULLUP); //use Arduinos built in pull up resistor-- by default we're at HIGH. 

  Serial.println("Inputs Initialized...");

  //configure ultrasonic pins

  pinMode(PIN_ULTRA_TRIG, OUTPUT);
  pinMode(PIN_ULTRA_ECHO, INPUT);
}

void manage_LED(bool isNewMaze){
  distance = get_distance();
  if(isNewMaze && distance <= DIST_THRESHOLD){
    digitalWrite(PIN_LED, HIGH);
    delay(100);
    digitalWrite(PIN_LED, LOW);
    delay(100);
    //TURN SCREEN ON 
    Serial.println("Screen ON!!");
  }else if ((isNewMaze && distance > DIST_THRESHOLD)){
    //don't turn screen on, but slow blink the LED
    digitalWrite(PIN_LED, HIGH);
    delay(200);
    digitalWrite(PIN_LED, LOW);
    delay(800);
  }else {
    digitalWrite(PIN_LED, LOW);
  }
}


// int get_xDirection(){
//   return;
// }
// int get_yDirection(){
//   return;

// }
// bool button_pressed(){
//   return;

// }

int get_distance(){

  //reset trig
  digitalWrite(PIN_ULTRA_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_ULTRA_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_ULTRA_TRIG, LOW);

  //read echo pin return sound wave travel time in us
  duration=pulseIn(PIN_ULTRA_ECHO,HIGH);

  // calculate distance in cm and print

  distance = (duration *0.034)/2;

  return distance;
}

// void trigger_LED(){
//   return;

// }