#include <FastLED.h>
#include <Arduino.h>

#include "AwsIotClient.h"
#include "SensorState.h"
#include "TelemetryPublisher.h"

// === LED Config ===
#define LED_PIN       6
#define NUM_LEDS      20
#define BRIGHTNESS    64
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB
CRGB leds[NUM_LEDS];

AwsIotClient aws;
TelemetryPublisher telemetry(aws);

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
int latestRaw[NUM_SENSORS];

// === Blink Config ===
const unsigned long BLINK_INTERVAL = 500;
unsigned long lastBlinkTime = 0;
bool blinkState = false;
const unsigned long TEST_HEARTBEAT_INTERVAL = 10000;
unsigned long lastTestHeartbeatTime = 0;
const unsigned long SENSOR_READ_INTERVAL = 1000;
unsigned long lastSensorReadTime = 0;

// === MQTT / State-change tracking ===
// Stores the last published state for each sensor so we can detect changes.
SensorState lastPublishedState[NUM_SENSORS];

// ---------------------------------------------------------
// Derive the current logical state from the two bool flags.
// ---------------------------------------------------------
SensorState getCurrentState(int s) {
  if (waterDetected[s]) return STATE_WET;
  if (drying[s])        return STATE_DRYING;
  return STATE_DRY;
}

// ---------------------------------------------------------
// Called whenever a sensor's state changes.
// ---------------------------------------------------------
void publishSensorState(int sensorId, SensorState state) {
  bool published = telemetry.publishSensorState(sensorId, state, latestRaw[sensorId]);
  Serial.print("[MQTT] Sensor ");
  Serial.print(sensorId);
  Serial.print(" changed -> ");
  Serial.print(sensorStateToString(state));
  Serial.print(" (");
  Serial.print(published ? "published" : "failed");
  Serial.println(")");
}

// ---------------------------------------------------------
// Check every sensor; publish only if the state has changed
// since the last publish.
// ---------------------------------------------------------
void checkAndPublishChanges() {
  for (int s = 0; s < NUM_SENSORS; s++) {
    SensorState current = getCurrentState(s);
    if (current != STATE_DRYING && current != lastPublishedState[s]) {
      publishSensorState(s, current);
      lastPublishedState[s] = current;
    }
  }
}

// -------------------------------------------------------
void updateLEDs() {
  for (int s = 0; s < NUM_SENSORS; s++) {
    int startLed = s * 5;
    bool active = waterDetected[s] || drying[s];
    for (int i = startLed; i < startLed + 5; i++) {
      if (active && blinkState) {
        leds[i] = CRGB::Red;
      } else {
        leds[i] = CRGB::Black;
      }
    }
  }
  FastLED.show();
}

void setup() {
  Serial.begin(9600);
  analogReadResolution(12);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  for (int s = 0; s < NUM_SENSORS; s++) {
    pinMode(SENSOR_PINS[s], INPUT);
    waterDetected[s]      = false;
    drying[s]             = false;
    historyIndex[s]       = 0;
    historyFilled[s]      = false;
    latestRaw[s]          = 0;
    lastPublishedState[s] = STATE_DRY;
    for (int i = 0; i < WINDOW_SIZE; i++) history[s][i] = 0;
  }

  if (!aws.begin()) {
    Serial.println("[AWS] Initial connection failed. Will keep retrying in loop.");
  } else {
    bool ok = telemetry.publishTestHeartbeat();
    Serial.println(ok ? "[MQTT] Test heartbeat published" : "[MQTT] Test heartbeat failed");
  }
}

void loop() {
  aws.loop();

  unsigned long now = millis();
  if (now - lastTestHeartbeatTime >= TEST_HEARTBEAT_INTERVAL) {
    lastTestHeartbeatTime = now;
    bool ok = telemetry.publishTestHeartbeat();
    Serial.println(ok ? "[MQTT] Test heartbeat published" : "[MQTT] Test heartbeat failed");
  }

  // --- Non-blocking blink toggle ---
  if (now - lastBlinkTime >= BLINK_INTERVAL) {
    lastBlinkTime = now;
    blinkState = !blinkState;
  }

  // --- Read all sensors and update state (once per second) ---
  if (now - lastSensorReadTime < SENSOR_READ_INTERVAL) return;
  lastSensorReadTime = now;

  for (int s = 0; s < NUM_SENSORS; s++) {
    int raw = analogRead(SENSOR_PINS[s]);
    latestRaw[s] = raw;

    history[s][historyIndex[s]] = raw;
    historyIndex[s] = (historyIndex[s] + 1) % WINDOW_SIZE;
    if (historyIndex[s] == 0) historyFilled[s] = true;

    int oldestIndex    = historyFilled[s] ? historyIndex[s] : 0;
    int dropOverWindow = history[s][oldestIndex] - raw;

    if (drying[s]) {
      if (raw <= THRESHOLD) {
        drying[s] = false;
      } else if (historyFilled[s] && raw > 500 && dropOverWindow <= 0) {
        drying[s]        = false;
        waterDetected[s] = true;
      }
    } else if (!waterDetected[s] && raw > THRESHOLD) {
      waterDetected[s] = true;
    } else if (waterDetected[s]) {
      if (raw < THRESHOLD) {
        waterDetected[s] = false;           // direct escape: below threshold → DRY
      } else if (historyFilled[s] && dropOverWindow > DRYING_RATE) {
        waterDetected[s] = false;
        drying[s]        = true;
      }
    }

    Serial.print("S");
    Serial.print(s);
    Serial.print(": ");
    Serial.print(raw);
    Serial.print(" ");
    Serial.print(waterDetected[s] ? "WATER" : (drying[s] ? "DRYING" : "DRY"));
    if (s < NUM_SENSORS - 1) Serial.print(" | ");
  }
  Serial.println();

  // --- Check for state changes and publish via MQTT ---
  checkAndPublishChanges();

  // --- Update LED strip ---
  updateLEDs();
}
