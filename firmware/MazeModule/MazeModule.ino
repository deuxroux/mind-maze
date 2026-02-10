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
  init_display();
  display_initial_maze();
  init_wifi();
  init_webapp();

}

void loop() {
  // put your main code here, to run repeatedly:
  serve_http();
  manage_LED(true);

}


void display_initial_maze() {
  init_maze(MAZE_DEFAULT_CELL_SIZE, MAZE_DEFAULT_WALL_WIDTH);
  regenerate_maze(MAZE_DEFAULT_SEED);
}