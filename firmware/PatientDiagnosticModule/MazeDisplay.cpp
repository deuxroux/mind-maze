#include "MazeDisplay.h"
#include "config.h"

Adafruit_SharpMem display(SHARP_SCK, SHARP_MOSI, SHARP_SS, 400, 240);
#define BLACK 0
#define WHITE 1


static const uint8_t BOTTOM = 0x01;
static const uint8_t RIGHT  = 0x02;
//allocate max mem cap
static const uint32_t MEM_MAX = 20000; 
uint8_t*  maze     = nullptr; // per-cell wall flags
uint16_t* mazepath = nullptr; 


int sizex = 0, sizey = 0;
int cellSize = 14;  // pixels / cell
int wallW   = 3;    // wall thickness (pixels)
int pathW = 18;           // corridor width
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
  
  sizex = display.width() / cellSize;
  sizey = display.height() / cellSize;

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
    if (i == 0)           maze[i] = 0;
    else if (i < (uint32_t)sizex) maze[i] = BOTTOM;         // top row
    else if ((i % sizex) == 0)    maze[i] = RIGHT;          // left column
    else                   maze[i] = (BOTTOM | RIGHT);      // interior
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
    maze[cell1] &= ~RIGHT;   // opened right wall
  } else {
    maze[cell1] &= ~BOTTOM;  // opened bottom wall
  }
}

// Try connecting a cell to either right or bottom neighbor if it merges components
bool connect_cell(int cell) {
  // skip borders (top row or left column)
  if (cell < sizex || (cell % sizex) == 0) return false;

  int tryOrder0 = random(2);         // 0 => right first, 1 => bottom first
  int tryOrder1 = (tryOrder0 + 1) % 2;

  int orders[2] = {tryOrder0, tryOrder1};

  for (int k = 0; k < 2; k++) {
    int which = orders[k];

    // which==0 -> right neighbor; which==1 -> bottom neighbor
    if (which == 0) {
      // don't open right edge
      if (GETX(cell) == sizex - 1) continue;
      int n = cell + 1;
      if (mazepath[cell] != mazepath[n]) {
        cell_join(cell, n);
        return true;
      }
    } else {
      // don't open bottom edge
      if (GETY(cell) == sizey - 1) continue;
      int n = cell + sizex;
      if (mazepath[cell] != mazepath[n]) {
        cell_join(cell, n);
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

    int cell = sizex + random(sizex * (sizey - 1)); // avoid top row
    for (int i = 0; i < sizex * sizey; i++) {
      int check = (i + cell) % (sizex * sizey);
      if (check < sizex || (check % sizex) == 0) continue;

      if (connect_cell(check)) {
        complete = false;
        break;
      }
    }
  } while (!complete);

  // Create entrance + exit by removing bottom wall on top and bottom rows (near center)
  int topOpen = (sizex / 4) + random(sizex / 2);
  maze[topOpen] &= ~BOTTOM;

  int botOpen = (sizex / 4) + random(sizex / 2) + sizex * (sizey - 1);
  maze[botOpen] &= ~BOTTOM;
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

  int mazePixW = sizex * pitch + wallW;
  int mazePixH = sizey * pitch + wallW;

  int x0 = (display.width()  - mazePixW) / 2;
  int y0 = (display.height() - mazePixH) / 2;

  // draw an outer border...
  // display.fillRect(x0, y0, mazePixW, wallW, BLACK);                      // top
  // display.fillRect(x0, y0 + mazePixH - wallW, mazePixW, wallW, BLACK);   // bottom
  // display.fillRect(x0, y0, wallW, mazePixH, BLACK);                      // left
  // display.fillRect(x0 + mazePixW - wallW, y0, wallW, mazePixH, BLACK);   // right

  for (int y = 0; y < sizey; y++) {
    for (int x = 0; x < sizex; x++) {
      int i = y * sizex + x;

      int px = x0 + x * pitch;
      int py = y0 + y * pitch;

      // bottom wall of cell i
      if (maze[i] & BOTTOM) {
        display.fillRect(px, py + pitch, pitch + wallW, wallW, BLACK);
      }
      // right wall of cell i
      if (maze[i] & RIGHT) {
        display.fillRect(px + pitch, py, wallW, pitch + wallW, BLACK);
      }
    }
  }
  display.refresh();
}
