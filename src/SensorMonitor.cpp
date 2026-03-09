#include "SensorMonitor.h"

#include <FastLED.h>

namespace {
constexpr int kLedPin = 6;
constexpr int kNumLeds = 20;
constexpr uint8_t kBrightness = 64;
constexpr int kLedsPerSensor = 5;

CRGB leds[kNumLeds];
}  // namespace

const int SensorMonitor::kSensorPins[SensorMonitor::kNumSensors] = {A0, A1, A2, A3};

void SensorMonitor::begin() {
  analogReadResolution(12);

  FastLED.addLeds<WS2812B, kLedPin, GRB>(leds, kNumLeds).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(kBrightness);
  FastLED.clear();
  FastLED.show();

  for (int s = 0; s < kNumSensors; s++) {
    pinMode(kSensorPins[s], INPUT);
    for (int i = 0; i < kWindowSize; i++) {
      history_[s][i] = 0;
    }
  }
}

void SensorMonitor::update() {
  const unsigned long now = millis();
  if (now - lastBlinkTime_ >= kBlinkIntervalMs) {
    lastBlinkTime_ = now;
    blinkState_ = !blinkState_;
  }

  for (int s = 0; s < kNumSensors; s++) {
    const int raw = analogRead(kSensorPins[s]);
    raw_[s] = raw;

    history_[s][historyIndex_[s]] = raw;
    historyIndex_[s] = (historyIndex_[s] + 1) % kWindowSize;
    if (historyIndex_[s] == 0) {
      historyFilled_[s] = true;
    }

    const int oldestIndex = historyFilled_[s] ? historyIndex_[s] : 0;
    const int dropOverWindow = history_[s][oldestIndex] - raw;

    if (drying_[s]) {
      if (raw <= kThreshold) {
        drying_[s] = false;
      } else if (historyFilled_[s] && raw > 500 && dropOverWindow <= 0) {
        drying_[s] = false;
        waterDetected_[s] = true;
      }
    } else if (!waterDetected_[s] && raw > kThreshold) {
      waterDetected_[s] = true;
    } else if (waterDetected_[s] && historyFilled_[s] && dropOverWindow > kDryingRate) {
      waterDetected_[s] = false;
      drying_[s] = true;
    }
  }

  updateLeds_();
}

void SensorMonitor::getSnapshot(SensorSnapshot out[kNumSensors]) const {
  for (int s = 0; s < kNumSensors; s++) {
    out[s] = {raw_[s], waterDetected_[s], drying_[s]};
  }
}

void SensorMonitor::updateLeds_() {
  for (int s = 0; s < kNumSensors; s++) {
    const int startLed = s * kLedsPerSensor;
    const bool active = waterDetected_[s] || drying_[s];

    for (int i = startLed; i < startLed + kLedsPerSensor; i++) {
      leds[i] = (active && blinkState_) ? CRGB::Red : CRGB::Black;
    }
  }
  FastLED.show();
}
