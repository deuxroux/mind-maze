#include <Arduino.h>
#include "LEDController.h"
#include "config.h"

int distance = 0;
long duration = 0;
const uint32_t DIST_SAMPLE_MS = 60; // how often to measure distance
const uint32_t ECHO_TIMEOUT_US = 25000; // ~25ms ~4m est range cap

uint32_t lastDistMs = 0;
int lastDistanceCm = -1;

struct LedPattern {
  const uint16_t* stepsMs; // alternating durations
  uint8_t count;
};

//define the on-off patterns for the far (noone nearby) states
const uint16_t PATTERN_READY_FAR[]     = { 100, 100, 100, 700}; // faster blink for ready
const uint16_t PATTERN_NOT_READY_FAR[] = { 100, 900};           // slow polling notif
const LedPattern P_READY_FAR      = { PATTERN_READY_FAR, 4 };
const LedPattern P_NOT_READY_FAR  = { PATTERN_NOT_READY_FAR, 2 };

static LedMode ledMode = LED_NOT_READY_NEAR;
static uint8_t ledStep = 0;
static bool ledOn = false;
static uint32_t ledStepStartMs = 0;

static bool mazeResultReady = false;
static bool mazeResultSlow = false;

const uint8_t LED_OFF = 255; //low values turn colors on, high values turn them off
const uint8_t LED_ON = 0;
const uint8_t READY_INTENSITY = 50; //tweak to make it mostly red or green but not harsh

//reusable-- writing the intensity of each color
static void write_rgb(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(RED_LED_PIN, r);
  analogWrite(GREEN_LED_PIN, g);
  analogWrite(BLUE_LED_PIN, b);
}

static void led_off() {
  write_rgb(LED_OFF, LED_OFF, LED_OFF);
}

static void apply_ready_color() {
  if (mazeResultSlow) {
    //slow completion write red
    write_rgb(READY_INTENSITY, LED_OFF, LED_OFF);
  } else {
    //fast completion write green
    write_rgb(LED_OFF, READY_INTENSITY, LED_OFF);
  }
}

static void apply_polling_color() {
  //white neutral polling inidicator only
  write_rgb(LED_ON, LED_ON, LED_ON);
}

static const LedPattern* current_pattern() {
  switch (ledMode) {
    case LED_READY_FAR: return &P_READY_FAR;
    case LED_NOT_READY_FAR: return &P_NOT_READY_FAR;
    default: return nullptr; //nothing if not clarified
  }
}

static void apply_mode_output() {
  if (ledMode == LED_READY_NEAR) {
    apply_ready_color();
    return;
  }
  if (ledMode == LED_NOT_READY_NEAR) {
    led_off();
    return;
  }
  if (ledMode == LED_READY_FAR) {
    if (ledOn) {
      apply_ready_color();
    } else {
      led_off();
    }
    return;
  }
  if (ledMode == LED_NOT_READY_FAR) {
    if (ledOn) {
      apply_polling_color();
    } else {
      led_off();
    }
    return;
  }
  led_off();
}

static int get_distance() {
  //reset trig
  digitalWrite(PIN_ULTRA_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_ULTRA_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_ULTRA_TRIG, LOW);

  //read echo pin return sound wave travel time in us-- gate using timeout
  duration = pulseIn(PIN_ULTRA_ECHO, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return -1; //timeout

  // calculate distance in cm
  distance = (duration * 0.034) / 2;
  return distance;
}

static bool update_distance(uint32_t nowMs) {
  if (nowMs - lastDistMs < DIST_SAMPLE_MS) return false;
  lastDistMs = nowMs;
  lastDistanceCm = get_distance();
  return true; // confirm if checked
}

static int last_distance_cm() {
  return lastDistanceCm;
}

void init_LED_controller() {
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  led_off();

  pinMode(PIN_ULTRA_TRIG, OUTPUT);
  pinMode(PIN_ULTRA_ECHO, INPUT);
}

static void set_led_mode(LedMode m, uint32_t nowMs) {
  if (m == ledMode) return;
  ledMode = m;
  ledStep = 0;
  ledOn = (m == LED_READY_FAR || m == LED_NOT_READY_FAR);
  ledStepStartMs = nowMs;
  apply_mode_output();
}

static void update_led(uint32_t nowMs) {
  if (ledMode == LED_READY_NEAR) {
    apply_ready_color();
    return;
  }
  if (ledMode == LED_NOT_READY_NEAR) {
    led_off();
    return;
  }

  const LedPattern* p = current_pattern();
  if (!p) {
    led_off();
    return;
  }

  uint16_t dur = p->stepsMs[ledStep];
  if (nowMs - ledStepStartMs < dur) return; //exit if not at poll point

  // advance to next step based on time pattern
  ledStepStartMs = nowMs;
  ledStep = (ledStep + 1) % p->count;

  ledOn = (ledStep % 2 == 0);
  apply_mode_output();
}

void set_maze_result_state(bool ready, uint32_t durationMs) {
  mazeResultReady = ready;
  if (ready) {
    mazeResultSlow = (durationMs > MAZE_COMPLETE_THRESHOLD_MS); //bad complete time
  } else {
    mazeResultSlow = false; //good complete time
  }
  apply_mode_output();
}

void update_led_controller(uint32_t nowMs) {
  update_distance(nowMs);
  const int latestDistance = last_distance_cm();
  const bool near = (latestDistance >= 0 && latestDistance <= DIST_THRESHOLD);

  if (mazeResultReady) {
    if (near) {
      set_led_mode(LED_READY_NEAR, nowMs);
    } else {
      set_led_mode(LED_READY_FAR, nowMs);
    }
  } else {
    if (near) {
      set_led_mode(LED_NOT_READY_NEAR, nowMs);
    } else {
      set_led_mode(LED_NOT_READY_FAR, nowMs);
    }
  }

  update_led(nowMs);
}