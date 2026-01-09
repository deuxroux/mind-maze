//hardware config lives here

#ifndef CONFIG_H
#define CONFIG_H

// --- Diagnostic Module Pinouts ---

// Joystick Pins (potentiometer Analog, switch digital)
#define PIN_JOYSTICK_X  A0
#define PIN_JOYSTICK_Y  A1
#define PIN_JOYSTICK_SW 2

// Sharp Memory Display Pins
#define PIN_DISP_SCK    13
#define PIN_DISP_MOSI   11
#define PIN_DISP_CS     10

// Ultrasonic Sensor
#define PIN_ULTRA_TRIG  7
#define PIN_ULTRA_ECHO  6

//notification LED
#define PIN_LED 4


#endif