#ifndef CONFIG_H
#define CONFIG_H

//--- Monitoring Mopdule Pinouts ---

//LCD pins


//ARGB pins
#define RED_LED_PIN D2
#define GREEN_LED_PIN D3
#define BLUE_LED_PIN D4


//ultrasonic pins
#define PIN_ULTRA_TRIG  D7
#define PIN_ULTRA_ECHO  D6

//ack button pin?

constexpr int DIST_THRESHOLD = 60; //60cm away triggers screen and led speed



#endif