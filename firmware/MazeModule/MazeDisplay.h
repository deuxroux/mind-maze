#ifndef MAZE_DISPLAY_H
#define MAZE_DISPLAY_H

#include <stdint.h> //c types for the maze generation logic
#include <stddef.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>

constexpr uint8_t MAZE_WALL_BOTTOM = 0x01;
constexpr uint8_t MAZE_WALL_RIGHT  = 0x02;
constexpr uint8_t MAZE_WALL_MASK   = MAZE_WALL_BOTTOM | MAZE_WALL_RIGHT; // or operator to combine the two. 

constexpr int MAZE_DEFAULT_CELL_SIZE  = 20;
constexpr int MAZE_DEFAULT_WALL_WIDTH = 3;
constexpr uint32_t MAZE_DEFAULT_SEED  = 4242; //random starting point. 

// Function prototypes from cpp file
void init_display(); //call the display to be initialized with appropriate pins.

void init_maze(int cellSize, int wallWidth);
void generate_maze();
void draw_maze_sharp();

void update_cursor(int x, int y);

void regenerate_maze(uint32_t seed = 0);
void display_current_maze();
void show_instruction_screen(const char *heading, const char *detail);
void show_completion_screen(uint32_t durationMs, bool sendingNow);
void set_cursor_graphic_visibility(bool visible);
bool maze_exit_reached();
uint32_t maze_exit_time_ms();

#endif
