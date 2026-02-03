#include "secrets.h"
#include "config.h"
#include "LEDController.h"
#include "ScreenManager.h"
#include "WebManager.h"

//init time and polling variables
const uint32_t PULL_INTERVAL_MS=5000; //check-in every 5s

uint32_t lastPull = 0;
bool haveMsg = false;

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
  serve_http();
  get_dist_and_notify();

  poll_peer();
}

void JSON_to_LCD(const JsonDocument &msg){
  String type = msg["type"] | "msg"; //default if nothing exists

  if(type == "maze_result"){
    bool success = msg["success"] | false;
    uint32_t dur = msg["duration_ms"] | 0; 
    const char* mazeId = msg["maze_id"] | ""; 

    String l1;
    String l2;
    //TODO: Update to manage the "time" requirement
    if(success){
      l1 = "Maze: OK";
    } else{
      l1 = "Maze: FAIL";
    }
    if (dur > 0){
      l2 = "t=" + fmtSeconds(dur);
    }else{
      l2 = "Error! t= ?";
    }
    //todo: add maze id? tbd if mportant here.
    update_screen(l1,l2);
    return;
  }
  //test payload case
  if (type == "test") {
    update_screen("RX: test", "peer json OK");
    return;
  }

}

String fmtSeconds(uint32_t ms) {
  // convert to X.Y seconds
  uint32_t tenths = ms / 100;
  String s = String(tenths / 10);
  s += ".";
  s += String(tenths % 10);
  s += "s";
  return s;
}

void poll_peer(){
  static bool showedDefault = false;

  if (millis() - lastPull >= PULL_INTERVAL_MS) {
    lastPull = millis();
  
    bool empty = true;
    StaticJsonDocument<512> msg;

    bool ok = pull_local_message(msg, empty);
    // Serial.print("OK: "); Serial.println(ok);
    // Serial.print("EMPTY: "); Serial.println(empty);
    //if(!ok) Serial.println("ERROR in pulling message");
    if (!empty && inboxUnread) {
      haveMsg = true;
      JSON_to_LCD(msg);
      inboxUnread = false;
      showedDefault = false;
      //Serial.println("[PULL] new message rendered to LCD");
    } 

    if(empty && haveMsg){
      haveMsg = false;  //clear flag
      update_screen("IDLE", "No Maze Active");
      showedDefault = true;
      return;
    }
  }

  // show something when no message yet and default has been shown
  if (!haveMsg && !showedDefault) {
    //set default text until maze updated
    update_screen("IDLE", "No Maze Active");
    showedDefault = true; 
  }
}
