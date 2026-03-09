#pragma once

#include <Arduino.h>

struct SensorSnapshot {
  int raw;
  bool waterDetected;
  bool drying;
};

class SensorMonitor {
public:
  static const int kNumSensors = 4;

  void begin();
  void update();
  void getSnapshot(SensorSnapshot out[kNumSensors]) const;

private:
  void updateLeds_();

  static const int kSensorPins[kNumSensors];
  static const int kThreshold = 1000;
  static const int kWindowSize = 10;
  static const int kDryingRate = 50;
  static const unsigned long kBlinkIntervalMs = 500;

  bool waterDetected_[kNumSensors] = {false, false, false, false};
  bool drying_[kNumSensors] = {false, false, false, false};
  int raw_[kNumSensors] = {0, 0, 0, 0};
  int history_[kNumSensors][kWindowSize] = {};
  int historyIndex_[kNumSensors] = {0, 0, 0, 0};
  bool historyFilled_[kNumSensors] = {false, false, false, false};

  unsigned long lastBlinkTime_ = 0;
  bool blinkState_ = false;
};
