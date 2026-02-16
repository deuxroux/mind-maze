#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

void init_inputs();

int get_xDirection();
int get_yDirection();
bool button_pressed();

int last_distance_cm();
bool update_distance(uint32_t nowMs);

enum LedMode : uint8_t { LED_OFF, LED_FAR, LED_NEAR };//define our enum class

void update_led(uint32_t nowMs);
void set_led_mode(LedMode m, uint32_t nowMs);

constexpr int DIST_THRESHOLD = 60; //60cm away triggers screen and led speed


#endif