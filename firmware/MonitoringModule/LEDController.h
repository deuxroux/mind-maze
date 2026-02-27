#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>

void init_LED_controller();

void update_led_controller(uint32_t nowMs);
void set_maze_result_state(bool ready, uint32_t durationMs);

bool button_pressed();

//reasonable maze completion time threshold (90s)
constexpr uint32_t MAZE_COMPLETE_THRESHOLD_MS = 90000;

//timing structure-- colors defined in cpp file
enum LedMode : uint8_t {
  LED_NOT_READY_NEAR, // no maze result and user nearby = LED off for no task to complete
  LED_NOT_READY_FAR,  // no maze result and user far = slow poll blink to show activity
  LED_READY_NEAR,     // maze result ready and user nearby = steady on for task indication
  LED_READY_FAR       // maze result ready and user far = faster double blink for alert
};

#endif
