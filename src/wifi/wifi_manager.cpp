#include "wifi_manager.h"

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"

bool connectWiFi() {
  Serial.println("Conectando ao Wi-Fi...");

  const unsigned long startTime = millis();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  bool connected = (WiFi.status() == WL_CONNECTED);
  while (!connected && millis() - startTime < WIFI_TIMEOUT_MS) {
    delay(100);
    connected = (WiFi.status() == WL_CONNECTED);
  }

  if (connected) {
    Serial.print("Wi-Fi conectado. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    WiFi.disconnect(true);
    Serial.println("Falha ao conectar ao Wi-Fi.");
  }
  return connected;
}
