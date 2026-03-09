#pragma once

enum SensorState { STATE_DRY, STATE_WET, STATE_DRYING };

inline const char* sensorStateToString(SensorState state) {
  switch (state) {
    case STATE_WET:
      return "WET";
    case STATE_DRYING:
      return "DRYING";
    default:
      return "DRY";
  }
}
