#include "wifi_manager.h"

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"

void connectWiFi() {
  Serial.println("Conectando ao Wi-Fi...");

  const unsigned long startTime = millis();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_TIMEOUT_MS) {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi conectado. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    WiFi.disconnect(true);
    Serial.println("Falha ao conectar ao Wi-Fi.");
  }
}
