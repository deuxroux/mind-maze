#include <Arduino.h>
#include "LEDController.h"
#include "config.h"

int distance = 0; 
long duration = 0;


void init_LED_controller(){

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN,OUTPUT);

  digitalWrite(BLUE_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, HIGH);

  pinMode(PIN_ULTRA_TRIG, OUTPUT);
  pinMode(PIN_ULTRA_ECHO, INPUT);

}

void manage_LED(int distance){
  if(distance <= DIST_THRESHOLD){
    analogWrite(BLUE_LED_PIN, 255);
    analogWrite(RED_LED_PIN, 255);
    analogWrite(GREEN_LED_PIN, 255);
    delay(100);
    analogWrite(BLUE_LED_PIN, 255);
    analogWrite(RED_LED_PIN, 50);
    analogWrite(GREEN_LED_PIN, 255);
    delay(100);
    //TURN SCREEN ON 
    //Serial.println("Screen ON!!");
  }else if (distance > DIST_THRESHOLD){
    //don't turn screen on, but slow blink the LED
    analogWrite(BLUE_LED_PIN, 255);
    analogWrite(RED_LED_PIN, 255);
    analogWrite(GREEN_LED_PIN, 255);
    delay(1000);
    analogWrite(BLUE_LED_PIN, 150);
    analogWrite(RED_LED_PIN, 150);
    analogWrite(GREEN_LED_PIN, 150);
    delay(500);
  }else {
    digitalWrite(BLUE_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, HIGH); 
  }
}


int get_dist_and_notify(){

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

  manage_LED(distance);
  return distance;
}