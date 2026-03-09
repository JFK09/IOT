#include <Arduino.h>
#include <WiFiNINA.h>

#include "secrets.h"

// Simple host to TCP-connect to as an internet reachability check
static const char CHECK_HOST[] = "www.google.com";
static const int  CHECK_PORT   = 80;

void setup() {
  Serial.begin(9600);
  while (!Serial) {}

  // --- 1. WiFi ---
  Serial.print("[WiFi] Connecting to \"");
  Serial.print(WIFI_SSID);
  Serial.print("\"");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - t0 > 20000UL) {
      Serial.println("\n[WiFi] FAILED (timeout)");
      return;
    }
  }
  Serial.println(" OK");
  Serial.print("[WiFi] IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("[WiFi] Signal (RSSI): ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  // --- 2. NTP time sync ---
  Serial.print("[NTP]  Syncing time");
  t0 = millis();
  while (WiFi.getTime() < 1609459200UL && (millis() - t0) < 15000UL) {
    delay(250);
    Serial.print(".");
  }
  if (WiFi.getTime() >= 1609459200UL) {
    Serial.print(" OK (epoch=");
    Serial.print(WiFi.getTime());
    Serial.println(")");
  } else {
    Serial.println(" FAILED (NTP timeout)");
  }

  // --- 3. Internet reachability via TCP ---
  Serial.print("[NET]  TCP connect to ");
  Serial.print(CHECK_HOST);
  Serial.print(":");
  Serial.print(CHECK_PORT);
  Serial.print(" ... ");
  WiFiClient client;
  if (client.connect(CHECK_HOST, CHECK_PORT)) {
    Serial.println("OK");
    client.stop();
  } else {
    Serial.println("FAILED");
  }

  Serial.println("\n[DONE] All checks complete.");
}

void loop() {}
