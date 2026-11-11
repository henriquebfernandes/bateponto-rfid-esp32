#include "wifi_manager.h"

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"

bool connectWiFi() {
  Serial.println("Conectando ao Wi-Fi...");

  // inicia a conexão com o Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 

  // aguarda a conexão até atingir o tempo limite
  const unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startedAt >= WIFI_TIMEOUT_MS) {
      WiFi.disconnect(true);
      Serial.println("Falha ao conectar ao Wi-Fi.");
      return false;
    }
    delay(100);
  }

  // exibe o IP após a conexão
  Serial.print("Wi-Fi conectado. IP: ");
  Serial.println(WiFi.localIP());
  return true;
}
