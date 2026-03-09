#include <Arduino.h>
#include <WiFiNINA.h>

#include "AwsConfig.h"
#include "AwsIotClient.h"
#include "SensorMonitor.h"

namespace {
AwsIotClient aws;
SensorMonitor sensors;

const unsigned long kPublishIntervalMs = 5000;
unsigned long lastPublishMs = 0;

bool buildJsonPayload(char* out, size_t outSize) {
  SensorSnapshot snapshot[SensorMonitor::kNumSensors];
  sensors.getSnapshot(snapshot);

  const unsigned long timestamp = WiFi.getTime();
  const int n = snprintf(
      out,
      outSize,
      "{\"deviceId\":\"%s\",\"timestamp\":%lu,\"sensors\":[{\"id\":0,\"raw\":%d,\"water\":%s,\"drying\":%s},"
      "{\"id\":1,\"raw\":%d,\"water\":%s,\"drying\":%s},"
      "{\"id\":2,\"raw\":%d,\"water\":%s,\"drying\":%s},"
      "{\"id\":3,\"raw\":%d,\"water\":%s,\"drying\":%s}]}",
      DEVICE_ID,
      timestamp,
      snapshot[0].raw, snapshot[0].waterDetected ? "true" : "false", snapshot[0].drying ? "true" : "false",
      snapshot[1].raw, snapshot[1].waterDetected ? "true" : "false", snapshot[1].drying ? "true" : "false",
      snapshot[2].raw, snapshot[2].waterDetected ? "true" : "false", snapshot[2].drying ? "true" : "false",
      snapshot[3].raw, snapshot[3].waterDetected ? "true" : "false", snapshot[3].drying ? "true" : "false");

  return n > 0 && static_cast<size_t>(n) < outSize;
}
}  // namespace

void setup() {
  Serial.begin(9600);
  while (!Serial) {
  }

  sensors.begin();
  if (!aws.begin()) {
    Serial.println("AWS init failed");
    while (true) delay(1000);
  }
}

void loop() {
  sensors.update();
  aws.loop();

  const unsigned long now = millis();
  if (now - lastPublishMs < kPublishIntervalMs) return;
  lastPublishMs = now;

  char payload[320];
  if (!buildJsonPayload(payload, sizeof(payload))) {
    Serial.println("Payload build failed");
    return;
  }

  if (aws.publishJson(TOPIC_PUB, payload)) {
    Serial.println(payload);
  } else {
    Serial.println("Publish failed");
  }
}
