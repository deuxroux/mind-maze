#include <Arduino.h>
#include "ScreenManager.h"
#include "config.h"
#include <string>

LiquidCrystal_I2C lcd(0x27, 16, 2);


void init_screen(){
  lcd.init();       
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("booting...");
}

void update_screen(const String& message){
  lcd.setCursor(0,0);
  lcd.print("monitoring...");
  lcd.setCursor(0,1);
  lcd.print(message);
}