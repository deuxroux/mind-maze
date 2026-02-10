#include "MazeDisplay.h"
#include "config.h"
#define BLACK 0
#define WHITE 1

Adafruit_SharpMem display(SHARP_SCK, SHARP_MOSI, SHARP_SS, 400, 240); //init the display


//NOTE: Most of below is Adafruit's example code for e-paper Maze Generator.
// VE has updated for SharpMem display and added QoL features for this application. 

//allocate max mem cap
static const uint32_t MEM_MAX = 20000; 
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
  // for (int k = 0; k < 2; k++) {
  //   int which = orders[k];

  //   // which==0 -> right neighbor; which==1 -> bottom neighbor
  //   if (which == 0) {
  //     // don't open right edge
  //     if (GETX(cell) == sizex - 1) continue;
  //     int n = cell + 1;
  //     if (mazepath[cell] != mazepath[n]) {
  //       cell_join(cell, n);
  //       return true;
  //     }
  //   } else {
  //     // don't open bottom edge
  //     if (GETY(cell) == sizey - 1) continue;
  //     int n = cell + sizex;
  //     if (mazepath[cell] != mazepath[n]) {
  //       cell_join(cell, n);
  //       return true;
  //     }
  //   }
  // }
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

// Render maze walls to SharpMem
void draw_maze_sharp() {
  display.clearDisplay();

  int frameW = sizex * pitch + wallW;
  int frameH = sizey * pitch + wallW;

  int fx = (display.width()  - frameW + 1) / 2;
  int fy = (display.height() - frameH + 1) / 2;

  const int leftX   = fx;
  const int topY    = fy;
  const int rightX  = fx + sizex * pitch;
  const int bottomY = fy + sizey * pitch;

  // safety check-- only generate if exists
  const int safeEntry = (entryX >= 0 && entryX < sizex) ? entryX : -1;
  const int safeExit  = (exitX  >= 0 && exitX  < sizex) ? exitX  : -1;

  //OUTER BORDERS
  //incl gap at entry at top/ compare against safety flags. 
  for (int x = 0; x < sizex; x++) {
    if (x == safeEntry) continue;
    thickH(leftX + x * pitch, topY, pitch, wallW, BLACK);
  }

  //bottom with gap
  for (int x = 0; x < sizex; x++) {
    if (x == safeExit) continue;
    thickH(leftX + x * pitch, bottomY, pitch, wallW, BLACK);
  }

  // Left and right borders
  thickV(leftX,  topY, wallW, sizey * pitch + wallW, BLACK);
  thickV(rightX, topY, wallW, sizey * pitch + wallW, BLACK);

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
  display.refresh();
}

// wrap maze generation logic in one function with seeding (for repeatability) 
void regenerate_maze(uint32_t seed) {
  if (seed != 0) randomSeed(seed);
  generate_maze();
  draw_maze_sharp();
}
