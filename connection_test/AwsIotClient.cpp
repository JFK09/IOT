#include "AwsIotClient.h"

#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>
#include <ArduinoBearSSL.h>
#include <BearSSLTrustAnchors.h>

#include "secrets.h"
#include "certs.h"
#include "AwsConfig.h"

static WiFiClient wifi;
static BearSSLClient ssl(wifi);
static MqttClient mqtt(ssl);

static unsigned long getTime() { return WiFi.getTime(); }

bool AwsIotClient::begin() {
  if (!connectWiFi_()) return false;
  if (!syncTime_()) return false;

  ArduinoBearSSL.onGetTime(getTime);

  // Trust AWS server + present device identity (mutual TLS)
  ssl.setTrustAnchors(TAs, TAs_NUM);
  ssl.setKey(DEVICE_KEY_PEM, DEVICE_CERT_PEM);

  mqtt.setId("MKR1010-WaterLevel");
  return connectMqtt_();
}

void AwsIotClient::loop() {
  mqtt.poll();
  if (!mqtt.connected()) {
    connectMqtt_(); // simple reconnect
  }
}

bool AwsIotClient::publishJson(const char* topic, const char* json) {
  if (!mqtt.connected()) return false;
  mqtt.beginMessage(topic);
  mqtt.print(json);
  mqtt.endMessage();
  return true;
}

bool AwsIotClient::connectWiFi_() {
  Serial.print("Connecting WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - t0 > 20000UL) return false;
  }
  Serial.println("\nWiFi connected");
  return true;
}

bool AwsIotClient::syncTime_() {
  unsigned long t0 = millis();
  while (WiFi.getTime() < 1609459200UL && (millis() - t0) < 15000UL) {
    delay(250);
  }
  return WiFi.getTime() >= 1609459200UL;
}

bool AwsIotClient::connectMqtt_() {
  Serial.print("Connecting AWS MQTT... ");
  if (!mqtt.connect(AWS_ENDPOINT, AWS_PORT)) {
    Serial.println("FAILED");
    Serial.print("MQTT err = "); Serial.println(mqtt.connectError());
    Serial.print("TLS err = ");  Serial.println(ssl.errorCode());
    return false;
  }
  Serial.println("OK");
  return true;
}