#include "config.h"
#include "InputHandler.h"
#include "MazeDisplay.h"
#include "WebManager.h"

bool isNewMaze = true; 

void setup() {
//CODE FOR SERIAL INITIALIZATION
  Serial.begin(9600);
  //delay for our serial monitor...
  delay(1200);
  Serial.println("\n[BOOT] Starting up…");
// END CODE FOR SERIAL INIT

//INSERT LATER 

  init_inputs();
  init_wifi();
  init_webapp();

}

void loop() {
  // put your main code here, to run repeatedly:

  serve_http();
  manage_LED(true);
  // delay(10000);
  // init_maze(24,3);
  // generate_maze();
  // draw_maze_sharp();
}
