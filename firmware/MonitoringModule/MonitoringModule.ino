#include "secrets.h"
#include "config.h"
#include "LEDController.h"
#include "ScreenManager.h"
#include "WebManager.h"

void setup() {
  //CODE FOR SERIAL INITIALIZATION
  Serial.begin(9600);
  //delay for our serial monitor...
  delay(1200);
  Serial.println("\n[BOOT] Starting up…");
// END CODE FOR SERIAL INIT

  init_wifi();

//INSERT LATER 
  init_LED_controller();
  init_screen();

  init_webapp();

}

void loop() {
  // put your main code here, to run repeatedly:
  int dist = get_dist_and_notify();
  update_screen("u have a page!");
  serve_http();
}
