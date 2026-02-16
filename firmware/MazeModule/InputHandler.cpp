#include <Arduino.h>
#include "InputHandler.h"
#include "config.h"


int distance = 0; 
long duration = 0;
static const uint32_t DIST_SAMPLE_MS = 60; // how often to measure distance
static const uint32_t ECHO_TIMEOUT_US = 25000; // ~25ms ~4m est range cap

static uint32_t lastDistMs = 0;
static int lastDistanceCm = -1;

struct LedPattern {
  const uint16_t* stepsMs; // alternating on-off durations
  uint8_t count;
};

//define the on-off patterns
const uint16_t PATTERN_FAR[]  = { 200, 800 };  // ON 200, OFF 800
const uint16_t PATTERN_NEAR[] = { 100, 100, 100, 700 }; //ON 100, OFF 100, ON 100, OFF 700
const LedPattern P_FAR  = { PATTERN_FAR,  2 };
const LedPattern P_NEAR = { PATTERN_NEAR, 4 };

static LedMode ledMode = LED_OFF;
static uint8_t ledStep = 0;
static bool ledOn = false;
static uint32_t ledStepStartMs = 0;

void init_inputs(){
  //configure LED pin
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);

  //configure joystick pins
  pinMode(PIN_JOYSTICK_X, INPUT);
  pinMode(PIN_JOYSTICK_Y, INPUT);
  pinMode(PIN_JOYSTICK_SW, INPUT_PULLUP); //use Arduinos built in pull up resistor-- by default we're at HIGH. 

  Serial.println("Inputs Initialized...");

  //configure ultrasonic pins

  pinMode(PIN_ULTRA_TRIG, OUTPUT);
  pinMode(PIN_ULTRA_ECHO, INPUT);
}

void set_led_mode(LedMode m, uint32_t nowMs) {
  if (m == ledMode) return;
  ledMode = m;
  ledStep = 0;
  ledOn = false;
  ledStepStartMs = nowMs;
  digitalWrite(PIN_LED, LOW);
}

//switch case to define pattern based on input by checking ledMode
const LedPattern* current_pattern() {
  switch (ledMode) {
    case LED_NEAR: return &P_NEAR;
    case LED_FAR:  return &P_FAR;
    default:       return nullptr;
  }
}

void update_led(uint32_t nowMs) {
  const LedPattern* p = current_pattern();
  if (!p) {
    digitalWrite(PIN_LED, LOW);
    return;
  }

  uint16_t dur = p->stepsMs[ledStep];
  if (nowMs - ledStepStartMs < dur) return;

  // advance to next step based on time prog
  ledStepStartMs = nowMs;
  ledStep = (ledStep + 1) % p->count;

  ledOn = (ledStep % 2 == 0);
  digitalWrite(PIN_LED, ledOn ? HIGH : LOW);
}

//convert joystick raw input -> step to move
static int joyToStep(int raw, bool invert) {
  int d = raw - JOYSTICK_CENTER;
  if (d > -DEADZONE && d < DEADZONE) return 0;

  if (d > (1023 - JOYSTICK_CENTER)) d = (1023 - JOYSTICK_CENTER);
  if (d < -JOYSTICK_CENTER) d = -JOYSTICK_CENTER;

  int mag = (d >= 0) ? d : -d; //define positive or negative movement
  int magOutside = mag - DEADZONE;

  int maxOutside = 0;
  //capture how far outside the deadzone the stick is
  if (d >= 0) {
    maxOutside = 1023 - JOYSTICK_CENTER - DEADZONE;
  }else {
    maxOutside = JOYSTICK_CENTER - DEADZONE;
  }
  if (maxOutside < 1) maxOutside = 1;  //ensure don't divide by zero

  int stepMag = map(magOutside, 0, maxOutside, 1, MAX_STEP); //map to max step param
  int step = 0;
  if (d >= 0) {
    step = stepMag;
  }else {
    step = -stepMag;
  }

  if (invert) step = -step;
  return step;
}

bool button_pressed(){
  return digitalRead(PIN_JOYSTICK_SW) == LOW;
}

int get_xDirection(){
  return joyToStep(analogRead(PIN_JOYSTICK_X), false);
}

int get_yDirection(){
  return joyToStep(analogRead(PIN_JOYSTICK_Y), false);
}

int get_distance(){
  //reset trig
  digitalWrite(PIN_ULTRA_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_ULTRA_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_ULTRA_TRIG, LOW);

  //read echo pin return sound wave travel time in us-- gate using global var
  duration=pulseIn(PIN_ULTRA_ECHO,HIGH,ECHO_TIMEOUT_US);
  if (duration == 0) return -1; //timout

  // calculate distance in cm and print
  distance = (duration *0.034)/2;

  return distance;
}

// check based on miliseconds since last check
bool update_distance(uint32_t nowMs) {
  if (nowMs - lastDistMs < DIST_SAMPLE_MS) return false;
  lastDistMs = nowMs;
  lastDistanceCm = get_distance();
  return true; // checked
}

//accessor
int last_distance_cm() {
  return lastDistanceCm;
}
