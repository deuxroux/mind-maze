#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H
#include <Arduino.h> 
#include <LiquidCrystal_I2C.h>

void init_screen();

void update_screen(const String& line1, const String& line2);

#endif