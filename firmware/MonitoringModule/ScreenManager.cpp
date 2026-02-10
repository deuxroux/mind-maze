#include <Arduino.h>
#include "ScreenManager.h"
#include "config.h"
#include <string>

LiquidCrystal_I2C lcd(0x27, 16, 2);

//Helper to manage buffer overflow for lcd screen
String fit16(const String& s) {
  if (s.length() >= 16) return s.substring(0, 16); //pull first 16 only
  String out = s;
  while (out.length() < 16) out += ' ';   // pad to overwrite
  return out;
}

void init_screen(){
  lcd.init();       
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Physician Monitor");
  lcd.setCursor(0,1);
  lcd.print("booting...");
}

void update_screen(const String& line1, const String& line2){
  lcd.setCursor(0,0);
  lcd.print(fit16(line1));
  lcd.setCursor(0,1);
  lcd.print(fit16(line2));
}