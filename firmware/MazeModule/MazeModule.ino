#include "config.h"
#include "InputHandler.h"
#include "MazeDisplay.h"
#include "WebManager.h"

bool isNewMaze = true;
uint32_t lastFrameMs = 0;

void setup() {
//CODE FOR SERIAL INITIALIZATION
  Serial.begin(9600);
  //delay for our serial monitor...
  delay(1200);
  Serial.println("\n[BOOT] Starting up…");
// END CODE FOR SERIAL INIT

//INSERT LATER 

  init_inputs();
  init_display();
  display_initial_maze();
  init_wifi();
  init_webapp();
  lastFrameMs = millis();

}

void loop() {
  const uint32_t now = millis(); //track current timestamp

  //sample distance at given interval
  if (update_distance(now)) {
    const int d = last_distance_cm();
    if (d < 0){
      set_led_mode(LED_OFF, now); // invalid reading need to debug
    } else if (d <=  DIST_THRESHOLD){ //60cm away triggers screen and led speed
      set_led_mode(LED_NEAR, now);
    } else {
      set_led_mode(LED_FAR, now);
    }
  }

  update_led(now);

  if (now - lastFrameMs >= FRAME_MS) {
    lastFrameMs = now;
    update_cursor(get_xDirection(), get_yDirection());
  }

  serve_http();

}


void display_initial_maze() {
  init_maze(MAZE_DEFAULT_CELL_SIZE, MAZE_DEFAULT_WALL_WIDTH);
  regenerate_maze(MAZE_DEFAULT_SEED);
}
