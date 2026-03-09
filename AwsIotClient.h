#pragma once
#include <Arduino.h>

class AwsIotClient {
public:
  bool begin();
  void loop();
  bool publishJson(const char* topic, const char* json);

private:
  bool connectWiFi_();
  bool syncTime_();
  bool connectMqtt_();
};