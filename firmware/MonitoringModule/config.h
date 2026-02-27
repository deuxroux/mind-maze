#ifndef CONFIG_H
#define CONFIG_H


//init time and polling variables
constexpr uint32_t ACK_DEBOUNCE_MS = 30; //TODO: prevent frequent polling of ack button
constexpr uint32_t PULL_INTERVAL_MS=5000; //check-in every 5s for lcd screen

//--- Monitoring Mopdule Pinouts ---

//LCD pins handled by i2c library


//ARGB pins
#define RED_LED_PIN D2
#define BLUE_LED_PIN D3
#define GREEN_LED_PIN D4


//ultrasonic pins
#define PIN_ULTRA_TRIG  D7
#define PIN_ULTRA_ECHO  D6

//ack button pin
#define ACK_BUTTON_PIN D1

constexpr int DIST_THRESHOLD = 60; //60cm away triggers screen and led speed


#define PEER_IP 192,168,86,20 // diagnostic modjule lives on 20
#define NODE_IP 192,168,86,21 //monitoring lives on 21
#define GATEWAY_IP 192,168,86,1
#define SUBNET_IP 255,255,255,0
#define DNS_IP 8,8,8,8



#endif