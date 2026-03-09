#include <Arduino.h>
#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>

#include <ArduinoBearSSL.h>
#include <BearSSLTrustAnchors.h>

#include "secrets.h"
#include "certs.h"   // ✅ add this

// Transport + TLS wrapper
WiFiClient wifiClient;
BearSSLClient sslClient(wifiClient);

MqttClient mqttClient(sslClient);

const char broker[] = "a1judrzsfnc856-ats.iot.eu-north-1.amazonaws.com";
const int port = 8883;
const char topic[] = "mkr1010/test";

unsigned long getTime() { return WiFi.getTime(); }

void setup() {
  Serial.begin(9600);
  while (!Serial) {}

  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Wait for a valid epoch from NTP. TLS cert validation needs real time.
  unsigned long t0 = millis();
  while (WiFi.getTime() < 1609459200UL && (millis() - t0) < 15000UL) {
    delay(250);
  }

  ArduinoBearSSL.onGetTime(getTime);

  // ✅ Trust AWS server (you can keep the built-in TAs)
  sslClient.setTrustAnchors(TAs, TAs_NUM);

  // ✅ Mutual TLS: present your device certificate + private key to AWS IoT
  // (key first, cert second)
  sslClient.setKey(DEVICE_KEY_PEM, DEVICE_CERT_PEM);

  mqttClient.setId("MKR1010Client");

  Serial.print("Connecting to AWS IoT MQTT... ");
  if (!mqttClient.connect(broker, port)) {
    Serial.println("FAILED");
    Serial.print("MQTT error = ");
    Serial.println(mqttClient.connectError());
    Serial.print("TLS error = ");
    Serial.println(sslClient.errorCode());
    while (1) delay(1000);
  }
  Serial.println("OK. Connected!");
}

void loop() {
  mqttClient.poll();

  mqttClient.beginMessage(topic);
  mqttClient.print("tick");
  mqttClient.endMessage();

  Serial.println("Published");
  delay(5000);
}