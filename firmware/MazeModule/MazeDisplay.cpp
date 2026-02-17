#include <Arduino.h>
#include "MazeDisplay.h"
#include "config.h"
#define BLACK 0
#define WHITE 1

Adafruit_SharpMem display(SHARP_SCK, SHARP_MOSI, SHARP_SS, 400, 240); //init the display

static float cursorX = 0.0f;
static float cursorY = 0.0f;
static bool cursorVisible = false;

static bool hasEntryZone = false;
static bool hasExitZone = false;
static int entryZoneLeft = 0;
static int entryZoneRight = 0;
static int entryZoneTop = 0;
static int entryZoneBottom = 0;
static int exitZoneLeft = 0;
static int exitZoneRight = 0;
static int exitZoneTop = 0;
static int exitZoneBottom = 0;

static int mazeLeftX = 0;
static int mazeRightX = 0;
static int mazeTopY = 0;
static int mazeBottomY = 0;

static bool hasEnteredMaze = false;
static bool hasExitedMaze = false;
static uint32_t mazeEnterMs = 0;
static uint32_t mazeExitMs = 0;


static const uint32_t MEM_MAX = 20000; //allocate max mem cap
uint8_t*  maze     = nullptr; // per-cell wall flags
uint16_t* mazepath = nullptr; 
int entryX = -1;  // column index for top opening-- init to -1
int exitX  = -1; 

int sizex = 0, sizey = 0;
// int cellSize = 9;  // pixels / cell
int wallW   = 3;    // wall thickness (pixels)
int pathW = 17;           // corridor width
int pitch = pathW + wallW;

#define IDX(x,y) ((y) * sizex + (x))
#define GETX(i)  ((i) % sizex)
#define GETY(i)  ((i) / sizex)

void init_display(){
  display.begin();
  display.clearDisplay();
  display.refresh();
}

//NOTE: Much of below is from Adafruit's example code for e-paper Maze Generator.
// VE has updated for SharpMem display and added QoL features for this application. 

void init_maze(int cellSize, int wallWidth){
  
  sizex = (display.width()  - wallW) / pitch;
  sizey = (display.height() - wallW) / pitch;

  //debug code
  Serial.print("[MAZE] pitch="); Serial.print(pitch);
  Serial.print(" wallW="); Serial.print(wallW);
  Serial.print(" frameW="); Serial.print(sizex*pitch + wallW);
  Serial.print(" frameH="); Serial.println(sizey*pitch + wallW);


  uint32_t cells = (uint32_t)sizex * (uint32_t)sizey;

  Serial.print("[MAZE] Generating grid: ");
  Serial.print(sizex);
  Serial.print(" x ");
  Serial.println(sizey);

  if (sizex < 4 || sizey < 4 || cells > MEM_MAX) {
    Serial.println("Invalid maze size; adjust cellSize.");
    while (1) delay(10);
  }

  // Allocate (or reallocate) arrays
  free(maze);
  free(mazepath);
  maze = (uint8_t*)malloc(cells * sizeof(uint8_t));
  mazepath = (uint16_t*)malloc(cells * sizeof(uint16_t));

  for (uint32_t i = 0; i < cells; i++) {
    maze[i] = MAZE_WALL_MASK; //use mask defined in .h file
    mazepath[i] = (uint16_t)i;
  }
}

// Union-like join: merge components; knock down wall between adjacent cells
void cell_join(int cell1, int cell2) {
  uint16_t oldVal = mazepath[cell2];
  uint16_t newVal = mazepath[cell1];

  // relabel all with oldVal -> newVal
  for (int i = sizex * sizey - 1; i >= 0; i--) {
    if (mazepath[i] == oldVal) mazepath[i] = newVal;
  }

  // remove the wall between them
  if (cell1 + 1 == cell2) {
    maze[cell1] &= ~MAZE_WALL_RIGHT;   // opened right wall
  } else {
    maze[cell1] &= ~MAZE_WALL_BOTTOM;  // opened bottom wall
  }
}

// Try connecting a cell to either right or bottom neighbor if it merges components
bool connect_cell(int cell) {

  int tryOrder0 = random(2);         // 0 => right first, 1 => bottom first
  int tryOrder1 = (tryOrder0 + 1) % 2;

  int orders[2] = {tryOrder0, tryOrder1};
  for (int k = 0; k < 2; k++) {
    int which = orders[k];

    if (which == 0) {
      if (GETX(cell) >= sizex - 1) continue; // dont go past right edge
      int n = cell + 1;
      if (mazepath[cell] != mazepath[n]) {
        cell_join(cell, n); // clears RIGHT on cell
        return true;
      }
    } else { // bottom
      if (GETY(cell) >= sizey - 1) continue; // don't go past bottom edge
      int n = cell + sizex;
      if (mazepath[cell] != mazepath[n]) {
        cell_join(cell, n); // clears BOTTOM on cell
        return true;
      }
    }
  }
  return false;
}

// Generate maze by repeatedly joining components until complete
void generate_maze() {
  bool complete;
  do {
    complete = true;
    int cell = sizex + random(sizex * (sizey - 1)); 
    for (int i = 0; i < sizex * sizey; i++) {
      int check = (i + cell) % (sizex * sizey);
      if (connect_cell(check)) {
        complete = false;
        break;
      }
    }
  } while (!complete);

  // Create entrance + exit by removing bottom wall on top and bottom rows (near center)
  entryX = (sizex / 4) + random(sizex / 2);
  exitX  = (sizex / 4) + random(sizex / 2);

  // For bottom opening last-row cell at exitX.
  int botCell = exitX + sizex * (sizey - 1);
  maze[botCell] &= ~MAZE_WALL_BOTTOM;
}

// Draw thick lines via fillRect (easier than manual multi-line thickness)
static inline void thickH(int x, int y, int w, int t, uint16_t color) {
  display.fillRect(x, y, w, t, color);
}
static inline void thickV(int x, int y, int t, int h, uint16_t color) {
  display.fillRect(x, y, t, h, color);
}

//END MAZE DISPLAy CODE FROM ADAFRUIT


// Render maze walls to SharpMem
//draw logic calls for display gfx library from Adafruit
void draw_maze_sharp() {
  display.clearDisplay();

  int frameW = sizex * pitch + wallW;
  int frameH = sizey * pitch + wallW;

  int fx = (display.width()  - frameW + 1) / 2;
  int fy = (display.height() - frameH + 1) / 2;

  mazeLeftX   = fx;
  mazeTopY    = fy;
  mazeRightX  = fx + sizex * pitch;
  mazeBottomY = fy + sizey * pitch;

  // safety check-- only generate if exists
  const int safeEntry = (entryX >= 0 && entryX < sizex) ? entryX : -1;
  const int safeExit  = (exitX  >= 0 && exitX  < sizex) ? exitX  : -1;

  //OUTER BORDERS
  //incl gap at entry at top/ compare against safety flags. 
  for (int x = 0; x < sizex; x++) {
    if (x == safeEntry) continue;
    int px = mazeLeftX + x * pitch;
    thickH(px, mazeTopY, pitch, wallW, BLACK);
  }

  //bottom with gap
  for (int x = 0; x < sizex; x++) {
    if (x == safeExit) continue;
    int px = mazeLeftX + x * pitch;
    thickH(px, mazeBottomY, pitch, wallW, BLACK);
  }

  // Left and right borders
  thickV(mazeLeftX, mazeTopY, wallW, sizey * pitch + wallW, BLACK);
  thickV(mazeRightX, mazeTopY, wallW, sizey * pitch + wallW, BLACK);

  for (int y = 0; y < sizey; y++) {
    for (int x = 0; x < sizex; x++) {
      int i = y * sizex + x;

      int px = fx + x * pitch;
      int py = fy + y * pitch;

      if (y < sizey - 1 && (maze[i] & MAZE_WALL_BOTTOM)) {
        display.fillRect(px, py + pitch, pitch + wallW, wallW, BLACK);
      }
      if (x < sizex - 1 && (maze[i] & MAZE_WALL_RIGHT)) {
        display.fillRect(px + pitch, py, wallW, pitch + wallW, BLACK);
      }
    }
  }

  int entryRowTop = fy + wallW;
  int exitRowTop = fy + (sizey - 1) * pitch + wallW;

  // entry/exit zones for boundary checking and timing
  if (safeEntry >= 0) {
    int px = fx + safeEntry * pitch + wallW;
    entryZoneLeft = px;
    entryZoneRight = px + pathW;
    entryZoneTop = entryRowTop;
    entryZoneBottom = entryRowTop + pathW;
    hasEntryZone = true;
  }else {
    hasEntryZone = false;
  }
  if (safeExit >= 0) {
    int px = fx + safeExit * pitch + wallW;
    exitZoneLeft = px;
    exitZoneRight = px + pathW;
    exitZoneTop = exitRowTop;
    exitZoneBottom = exitRowTop + pathW;
    hasExitZone = true;
  }else {
    hasExitZone = false;
  }

  display.refresh();
}

//check collision states
static inline bool rects_overlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
  return !(ax + aw <= bx || ax >= bx + bw || ay + ah <= by || ay >= by + bh);
}

static inline int clamp_axis(int value, int lo, int hi) {
  if (value < lo) return lo;
  if (value > hi) return hi;
  return value;
}

static bool cursor_hits_wall(int x, int y) {
  const int safeEntry = (entryX >= 0 && entryX < sizex) ? entryX : -1;
  const int safeExit  = (exitX  >= 0 && exitX  < sizex) ? exitX  : -1;

  auto overlaps_wall = [&](int wx, int wy, int ww, int wh) {
    return rects_overlap(x, y, CURSOR_WIDTH, CURSOR_HEIGHT, wx, wy, ww, wh);
  };

  for (int cx = 0; cx < sizex; cx++) {
    int px = mazeLeftX + cx * pitch;
    if (cx != safeEntry && overlaps_wall(px, mazeTopY, pitch, wallW)) {
      return true;
    }
    if (cx != safeExit && overlaps_wall(px, mazeBottomY, pitch, wallW)) {
      return true;
    }
  }

  if (overlaps_wall(mazeLeftX, mazeTopY, wallW, sizey * pitch + wallW)) return true;
  if (overlaps_wall(mazeRightX, mazeTopY, wallW, sizey * pitch + wallW)) return true;

  for (int cy = 0; cy < sizey; cy++) {
    int py = mazeTopY + cy * pitch;
    for (int cx = 0; cx < sizex; cx++) {
      int i = cy * sizex + cx;
      int px = mazeLeftX + cx * pitch;
      if ((maze[i] & MAZE_WALL_BOTTOM) && overlaps_wall(px, py + pitch, pitch + wallW, wallW)) {
        return true;
      }
      if ((maze[i] & MAZE_WALL_RIGHT) && overlaps_wall(px + pitch, py, wallW, pitch + wallW)) {
        return true;
      }
    }
  }

  return false;
}

static void draw_cursor(int x, int y) {
  display.fillRect(x, y, CURSOR_WIDTH, CURSOR_HEIGHT, BLACK);
}

static void erase_cursor(int x, int y) {
  display.fillRect(x, y, CURSOR_WIDTH, CURSOR_HEIGHT, WHITE);
}

static inline bool cursor_in_zone(int x, int y, int left, int right, int top, int bottom) {
  return !(x + CURSOR_WIDTH <= left || x >= right || y + CURSOR_HEIGHT <= top || y >= bottom);
}

//define entry and exit flags
static void update_entry_exit_state(uint32_t now) {
  if (hasEntryZone && !hasEnteredMaze &&
      cursor_in_zone((int)cursorX, (int)cursorY, entryZoneLeft, entryZoneRight, entryZoneTop, entryZoneBottom)) {
    hasEnteredMaze = true;
    mazeEnterMs = now;
  }
  if (hasExitZone && hasEnteredMaze && !hasExitedMaze &&
      cursor_in_zone((int)cursorX, (int)cursorY, exitZoneLeft, exitZoneRight, exitZoneTop, exitZoneBottom)) {
    hasExitedMaze = true;
    mazeExitMs = now;
  }
}

//code to adjust position and visibility of cursor
void reset_cursor_position() {
  if (cursorVisible) {
    erase_cursor((int)cursorX, (int)cursorY);
  }
  cursorVisible = false;
  hasEnteredMaze = false;
  hasExitedMaze = false;
  mazeEnterMs = 0;
  mazeExitMs = 0;

  int startX = (display.width() - CURSOR_WIDTH) / 2;
  int startY = 0;
  if (hasEntryZone) {
    int zoneWidth = entryZoneRight - entryZoneLeft;
    startX = entryZoneLeft + (zoneWidth - CURSOR_WIDTH) / 2;
    startY = entryZoneTop - CURSOR_HEIGHT - 2;
  }

  startX = clamp_axis(startX, 0, display.width() - CURSOR_WIDTH);
  startY = clamp_axis(startY, 0, display.height() - CURSOR_HEIGHT);

  cursorX = startX;
  cursorY = startY;
  cursorVisible = true;
  draw_cursor(startX, startY);
  display.refresh();
  update_entry_exit_state(millis());
}

void update_cursor(int dx, int dy) {
  if (!cursorVisible) return;

  int baseX = (int)cursorX;
  int baseY = (int)cursorY;
  int candidateX = baseX;
  int candidateY = baseY;

  if (dx != 0) {
    int nextX = clamp_axis(baseX + dx, 0, display.width() - CURSOR_WIDTH);
    if (!cursor_hits_wall(nextX, baseY)) {
      candidateX = nextX;
    }
  }

  if (dy != 0) {
    int nextY = clamp_axis(baseY + dy, 0, display.height() - CURSOR_HEIGHT);
    if (!cursor_hits_wall(candidateX, nextY)) {
      candidateY = nextY;
    }else if (candidateX != baseX && !cursor_hits_wall(baseX, nextY)) {
      candidateX = baseX;
      candidateY = nextY;
    }
  }

  if (candidateX == baseX && candidateY == baseY) return;

  erase_cursor(baseX, baseY);
  cursorX = candidateX;
  cursorY = candidateY;
  draw_cursor(candidateX, candidateY);
  display.refresh();
  update_entry_exit_state(millis());
}

// wrap maze generation logic in one function with seeding (for repeatability) 
void regenerate_maze(uint32_t seed) {
  if (seed != 0) randomSeed(seed);
  generate_maze();
}

void display_current_maze() {
  draw_maze_sharp();
  reset_cursor_position();
}

void set_cursor_graphic_visibility(bool visible) {
  if (visible && !cursorVisible && cursorX <= display.width() && cursorY <= display.height()) {
    draw_cursor((int)cursorX, (int)cursorY);
    cursorVisible = true;
    display.refresh();
    return;
  }
  if (!visible && cursorVisible) {
    erase_cursor((int)cursorX, (int)cursorY);
    cursorVisible = false;
    display.refresh();
  }
}

static void draw_wrapped_text(int x, int y, int size, const char *text) {
  display.setTextSize(size);
  display.setCursor(x, y);
  display.print(text);
}

void show_instruction_screen(const char *heading, const char *detail) {
  display.clearDisplay();
  display.setTextColor(BLACK);
  const int xMargin = 8; //defines padding
  const int yMargin = 48;
  draw_wrapped_text(xMargin, yMargin, 3, heading);
  display.setTextSize(1);
  draw_wrapped_text(xMargin, yMargin + 60, 2, detail);
  display.refresh();
}

void show_completion_screen(uint32_t durationMs, bool sendingNow) {
  uint32_t tenths = durationMs / 100;
  String detail = String(tenths / 10);
  detail += ".";
  detail += String(tenths % 10);
  detail += "s";
  display.clearDisplay();
  display.setTextColor(BLACK);
  const int margin = 8;
  draw_wrapped_text(margin, margin, 3, "Maze Complete!");
  display.setTextSize(1);
  draw_wrapped_text(margin, margin + 60, 2, detail.c_str()); //expects char so cast as c_str
  if (sendingNow) {
    draw_wrapped_text(margin, margin + 90, 2, "Sending results to monitor");
  } else {
    draw_wrapped_text(margin, margin + 90, 2, "Results will be sent shortly");
  }
  display.refresh();
}

bool maze_exit_reached() {
  return hasExitedMaze;//accessor for this var
}

uint32_t maze_exit_time_ms() {
  if (!hasExitedMaze || mazeExitMs <= mazeEnterMs) return 0;
  return mazeExitMs - mazeEnterMs;
}
