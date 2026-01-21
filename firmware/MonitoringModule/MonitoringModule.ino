#include <aWOT.h>
#include <WiFiS3.h>
#include "secrets.h"
#include "config.h"
#include "LEDController.h"
#include "ScreenManager.h"


WiFiServer server(80);
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
awot::Application app;

void setup() {
//CODE FOR SERIAL INITIALIZATION
  Serial.begin(9600);
  //delay for our serial monitor...
  delay(1200);
  Serial.println("\n[BOOT] Starting up…");
// END CODE FOR SERIAL INIT

//INSERT LATER 
  init_LED_controller();
  init_screen();

//CODE FOR WIFI INIT
  WiFi.begin(ssid, pass);
  delay(1000);
  //check if module exists
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("[ERROR] WiFiS3 module not detected.");
  }
  Serial.print("[WiFi] Attempting connection to SSID: ");
  Serial.println(ssid);
  //Wait for connection...
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.println("Waiting to connect...");
  }

  Serial.print("[WiFi] Connected! IP Address: ");
  Serial.println(WiFi.localIP());
//END CODE FOR WIFI INIT
}

void loop() {
  // put your main code here, to run repeatedly:
  int dist = get_dist_and_notify();
  update_screen("u have a page!");
}
