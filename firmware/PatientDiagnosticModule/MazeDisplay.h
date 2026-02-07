#ifndef MAZE_DISPLAY_H
#define MAZE_DISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>

// Function prototypes from cpp file
void init_display(); //call the display to be initialized with appropriate pins.

void init_maze(int cellSize, int wallWidth);
void generate_maze();
void draw_maze_sharp();

void update_cursor(int x, int y);

#endif