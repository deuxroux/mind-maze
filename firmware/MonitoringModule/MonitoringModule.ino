#include "secrets.h"
#include "config.h"
#include "LEDController.h"
#include "ScreenManager.h"
#include "WebManager.h"

static bool lastInboxUnread = false;

static void sync_inbox_led_state();


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
  pinMode(ACK_BUTTON_PIN, INPUT);
}

void loop() {
  serve_http();
  render_inbox_if_pending(); 
  update_led_controller(millis());
  poll_peer();
  handle_ack_button();
  sync_inbox_led_state();
}

void JSON_to_LCD(const JsonDocument &msg){
  String type = msg["type"] | "msg"; //default if nothing exists

  if(type == "maze_result"){
    String status = msg["status"] | "";
    bool success = msg["success"] | false;
    uint32_t dur = msg["duration_ms"] | 0; //0 means incomplete
    const char* mazeId = msg["maze_id"] | ""; 

    String l1;
    String l2;
    //TODO: Update to manage the "time" requirement
    if (status == "incomplete") {
      l1 = "Maze: INCOMP";
    } else if (status == "failure" || (!status.length() && !success)) {
      l1 = "Maze: FAIL";
    } else {
      l1 = "Maze: OK";
    }
    if (dur > 0){
      l2 = "t=" + fmt_seconds(dur);
    }else if (status == "incomplete"){
      l2 = "t=?.?s";
    }else{
      l2 = "Error! t= ?";
    }
    //todo: add maze id? tbd if mportant here.
    update_screen(l1,l2);
    mazeAwaiting = false;
    set_maze_result_state(true, dur);
    return;
  }
  //test payload case
  if (type == "test") {
    update_screen("RX: test", "peer json OK");
    return;
  }

}

String fmt_seconds(uint32_t ms) {
  // convert to X.Y seconds
  uint32_t tenths = ms / 100;
  String s = String(tenths / 10);
  s += ".";
  s += String(tenths % 10);
  s += "s";
  return s;
}

void poll_peer(){
  static uint32_t lastPull = 0;

  if (millis() - lastPull < PULL_INTERVAL_MS) return;
  lastPull = millis();

  if (inboxRenderPending) return;
  if (inboxUnread) return;

  if (mazeAwaiting) {
    if (inboxUnread) return;
    update_screen("AWAITING", "No Result Yet");
  } else { 
    update_screen("READY", "Send New Maze");
  }

  Serial.print("[POLL] mazeAwaiting=");
  Serial.print(mazeAwaiting);
  Serial.print(" inboxUnread=");
  Serial.print(inboxUnread);
  Serial.print(" pending=");
  Serial.println(inboxRenderPending);
}

void handle_ack_button() {
  int reading = digitalRead(ACK_BUTTON_PIN);
  if (!reading) {
    Serial.println("Ack button pressed");
    clear_inbox_state();
  }
}

static void sync_inbox_led_state() {
  if (lastInboxUnread && !inboxUnread) {
    set_maze_result_state(false, 0);
  }
  lastInboxUnread = inboxUnread;
}

static void render_inbox_if_pending() {
  if (!inboxRenderPending) return;

  StaticJsonDocument<512> msg;
  bool empty = true;
  bool ok = pull_local_message(msg, empty);

  if (ok && !empty) {
    JSON_to_LCD(msg);
    inboxRenderPending = false;
  }
}
