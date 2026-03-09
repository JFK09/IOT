#pragma once

#include <Arduino.h>

#include "AwsIotClient.h"
#include "SensorState.h"

class TelemetryPublisher {
public:
  explicit TelemetryPublisher(AwsIotClient& awsClient);
  bool publishSensorState(int sensorId, SensorState state, int rawValue);
  bool publishTestHeartbeat();

private:
  AwsIotClient& awsClient_;
};
