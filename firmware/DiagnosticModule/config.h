//hardware config lives here

#ifndef CONFIG_H
#define CONFIG_H

// --- Diagnostic Module Pinouts ---

// Joystick Pins (potentiometer Analog, switch digital)
#define PIN_JOYSTICK_X  A0
#define PIN_JOYSTICK_Y  A1
#define PIN_JOYSTICK_SW D2

// Sharp Memory Display Pins
#define SHARP_SCK  D13
#define SHARP_MOSI D11
#define SHARP_SS   D10

// Ultrasonic Sensor
#define PIN_ULTRA_TRIG  D7
#define PIN_ULTRA_ECHO  D6

//notification LED
#define PIN_LED D4

#define NODE_IP 192,168,86,20 //diagnostic mod will have 20
#define PEER_IP 192,168,86,21 //monitoring will have 21
#define GATEWAY_IP 192,168,86,1
#define SUBNET_IP 255,255,255,0
#define DNS_IP 8,8,8,8

#endif