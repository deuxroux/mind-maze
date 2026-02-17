#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

void init_inputs();

int get_xDirection();
int get_yDirection();
bool button_pressed();

int last_distance_cm();
bool update_distance(uint32_t nowMs);

enum LedMode : uint8_t {
  LED_NOT_READY_NEAR, // no maze and user nearby = LED off for no task to complete
  LED_NOT_READY_FAR,  // no maze and user far = slow poll blink to show activity
  LED_READY_NEAR,     // maze ready and user nearby = steady on for task inidication
  LED_READY_FAR       // maze ready and user far = faster double blink for alert
};

void update_led(uint32_t nowMs);
void set_led_mode(LedMode m, uint32_t nowMs);

constexpr int DIST_THRESHOLD = 60; //dist away that triggers led speed


#endif
