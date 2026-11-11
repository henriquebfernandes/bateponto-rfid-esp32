#include <Arduino.h>

#include "config.h"
#include "wifi/wifi_manager.h"

bool setup_success = false;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);

  if (!connectWiFi()) {return;}
  
  setup_success = true;
}

void loop() {
  if (!setup_success) {
    Serial.println("Setup falhou. Reiniciando...");
    delay(5000);
    ESP.restart();
  }

  // Coloque aqui o código principal do seu programa
}
