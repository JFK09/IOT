#include <FastLED.h>
#include <Arduino.h>
#include "AwsIotClient.h"
#include "AwsConfig.h"

AwsIotClient aws;

// === LED Config ===
#define LED_PIN       6
#define NUM_LEDS      20      // 4 sensors × 5 LEDs each
#define BRIGHTNESS    64
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB

CRGB leds[NUM_LEDS];

// === Sensor Config ===
const int NUM_SENSORS  = 4;
const int SENSOR_PINS[NUM_SENSORS] = {A0, A1, A2, A3};
const int THRESHOLD    = 1000;
const int WINDOW_SIZE  = 10;
const int DRYING_RATE  = 50;

bool waterDetected[NUM_SENSORS];
bool drying[NUM_SENSORS];
int  history[NUM_SENSORS][WINDOW_SIZE];
int  historyIndex[NUM_SENSORS];
bool historyFilled[NUM_SENSORS];

// === Blink Config ===
const unsigned long BLINK_INTERVAL = 500; // ms
unsigned long lastBlinkTime = 0;
bool blinkState = false;  // true = LEDs on, false = LEDs off

// -------------------------------------------------------
// Maps each sensor to its 5-LED group and sets the color
// based on the current blink state and sensor status
// -------------------------------------------------------
void updateLEDs() {
  for (int s = 0; s < NUM_SENSORS; s++) {
    int startLed = s * 5;                            // sensor 0→0, 1→5, 2→10, 3→15
    bool active = waterDetected[s] || drying[s];     // blink if wet OR drying

    for (int i = startLed; i < startLed + 5; i++) {
      if (active && blinkState) {
        leds[i] = CRGB::Red;   // wet/drying + blink ON  → red
      } else {
        leds[i] = CRGB::Black; // dry, or blink OFF phase → off
      }
    }
  }
  FastLED.show();
}

void setup() {
  Serial.begin(9600);
  analogReadResolution(12);

  // Init LEDs
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  // Init sensors
  for (int s = 0; s < NUM_SENSORS; s++) {
    pinMode(SENSOR_PINS[s], INPUT);
    waterDetected[s] = false;
    drying[s]        = false;
    historyIndex[s]  = 0;
    historyFilled[s] = false;
    for (int i = 0; i < WINDOW_SIZE; i++) history[s][i] = 0;
  }
}

void loop() {
  // --- Non-blocking blink toggle ---
  unsigned long now = millis();
  if (now - lastBlinkTime >= BLINK_INTERVAL) {
    lastBlinkTime = now;
    blinkState = !blinkState;
  }

  // --- Read all sensors and update state ---
  for (int s = 0; s < NUM_SENSORS; s++) {
    int raw = analogRead(SENSOR_PINS[s]);

    // Store in circular buffer
    history[s][historyIndex[s]] = raw;
    historyIndex[s] = (historyIndex[s] + 1) % WINDOW_SIZE;
    if (historyIndex[s] == 0) historyFilled[s] = true;

    // Calculate drop over the window
    int oldestIndex    = historyFilled[s] ? historyIndex[s] : 0;
    int dropOverWindow = history[s][oldestIndex] - raw;

    // State machine (unchanged from your original)
    if (drying[s]) {
      if (raw <= THRESHOLD) {
        drying[s] = false;
      } else if (historyFilled[s] && raw > 500 && dropOverWindow <= 0) {
        drying[s]        = false;
        waterDetected[s] = true;
      }
    } else if (!waterDetected[s] && raw > THRESHOLD) {
      waterDetected[s] = true;
    } else if (waterDetected[s] && historyFilled[s]) {
      if (dropOverWindow > DRYING_RATE) {
        waterDetected[s] = false;
        drying[s]        = true;
      }
    }

    // Serial output
    Serial.print("S");
    Serial.print(s);
    Serial.print(": ");
    Serial.print(raw);
    Serial.print(" ");
    Serial.print(waterDetected[s] ? "WATER" : (drying[s] ? "DRYING" : "DRY"));
    if (s < NUM_SENSORS - 1) Serial.print(" | ");
  }
  Serial.println();

  // --- Update LED strip ---
  updateLEDs();
}