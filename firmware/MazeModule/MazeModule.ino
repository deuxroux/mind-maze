#include "config.h"
#include "MazeModule.h"
#include "InputHandler.h"
#include "MazeDisplay.h"
#include "WebManager.h"

enum MazeSessionState : uint8_t {
  MAZE_STATE_IDLE,
  MAZE_STATE_READY,
  MAZE_STATE_RUNNING,
  MAZE_STATE_COMPLETED
};

static MazeSessionState mazeState = MAZE_STATE_IDLE;
static bool instructionNeedsUpdate = true;
static uint32_t completionDisplayStartMs = 0;
static uint32_t mazeStartMs = 0;
static uint32_t mazeDurationMs = 0;
static uint32_t lastFrameMs = 0;
static bool buttonPreviouslyPressed = false;

static void update_instruction_screen();
static void update_led_for_state(uint32_t now, bool near);
static void begin_maze_run(uint32_t now);
static void finalize_maze_run(uint32_t now);

void setup() {
  Serial.begin(9600);
  delay(1200);
  Serial.println("\n[BOOT] Starting up…");

  init_inputs();
  init_display();
  display_initial_maze();
  init_wifi();
  init_webapp();
  lastFrameMs = millis();
}

void loop() {
  const uint32_t now = millis(); // track current time each frame

  update_distance(now);
  const int latestDistance = last_distance_cm();
  const bool near = (latestDistance >= 0 && latestDistance <= DIST_THRESHOLD);
  update_led_for_state(now, near);
  update_led(now);

  if (mazeActive && mazeState == MAZE_STATE_IDLE) {
    mazeState = MAZE_STATE_READY;
    instructionNeedsUpdate = true;
  } else if (!mazeActive && mazeState == MAZE_STATE_READY) {
    mazeState = MAZE_STATE_IDLE;
    instructionNeedsUpdate = true;
  }

  if (mazeState == MAZE_STATE_RUNNING) {
    if (now - lastFrameMs >= FRAME_MS) {
      lastFrameMs = now;
      update_cursor(get_xDirection(), get_yDirection());
      if (maze_exit_reached()) {
        finalize_maze_run(now);
      }
    }
  } else if (mazeState == MAZE_STATE_COMPLETED) {
    if (completionDisplayStartMs && (now - completionDisplayStartMs >= COMPLETION_DISPLAY_MS)) {
      mazeState = MAZE_STATE_IDLE;
      instructionNeedsUpdate = true;
      completionDisplayStartMs = 0;
    }
  } else {
    if (instructionNeedsUpdate) {
      update_instruction_screen();
    }
  }

  const bool buttonNow = button_pressed();
  if (mazeState == MAZE_STATE_READY && buttonNow && !buttonPreviouslyPressed) {
    begin_maze_run(now);
  }
  buttonPreviouslyPressed = buttonNow;

  serve_http();
}

void display_initial_maze() {
  init_maze(MAZE_DEFAULT_CELL_SIZE, MAZE_DEFAULT_WALL_WIDTH);
  regenerate_maze(MAZE_DEFAULT_SEED);
  set_cursor_graphic_visibility(false);
  mazeState = MAZE_STATE_IDLE;
  instructionNeedsUpdate = true;
  completionDisplayStartMs = 0;
  mazeStartMs = 0;
  mazeDurationMs = 0;
  update_instruction_screen();
}

static void update_instruction_screen() {
  set_cursor_graphic_visibility(false);
  if (mazeState == MAZE_STATE_READY) {
    show_instruction_screen("Maze Ready!", ">> Press switch to begin <<");
  } else {
    show_instruction_screen("No Maze Assigned.", ">> Come Back Later <<");
  }
  instructionNeedsUpdate = false;
}

static void update_led_for_state(uint32_t now, bool near) {
  if (mazeState == MAZE_STATE_READY) {
    if (near) {
      set_led_mode(LED_READY_NEAR, now);
    } else {
      set_led_mode(LED_READY_FAR, now);
    }
  } else {
    if (near) {
      set_led_mode(LED_NOT_READY_NEAR, now);
    } else {
      set_led_mode(LED_NOT_READY_FAR, now);
    }
  }
}

static void begin_maze_run(uint32_t now) {
  mazeState = MAZE_STATE_RUNNING;
  mazeStartMs = now;
  mazeDurationMs = 0;
  completionDisplayStartMs = 0;
  lastFrameMs = now;
  set_cursor_graphic_visibility(false);
  display_current_maze();
  mazeActive = false;
}

static void finalize_maze_run(uint32_t now) {
  if (mazeState != MAZE_STATE_RUNNING) return;
  mazeDurationMs = now - mazeStartMs;
  mazeState = MAZE_STATE_COMPLETED;
  completionDisplayStartMs = now;
  set_cursor_graphic_visibility(false);
  bool sendOk = send_maze_result_payload(mazeDurationMs);
  show_completion_screen(mazeDurationMs, sendOk);
}
