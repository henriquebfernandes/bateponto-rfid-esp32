#include <Arduino.h>

#include "clock/clock_sync.h"
#include "config.h"
#include "signaling/signaling.h"
#include "wifi/wifi_manager.h"

namespace {
constexpr unsigned long RESTART_DELAY_MS = 5000;
bool setup_success = false;
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);

  setup_success = beginSignaling() && connectWiFi() && syncClock();
  signalResult(setup_success);

  if (!setup_success) {
    Serial.println("Setup falhou. Reiniciando...");
    delay(RESTART_DELAY_MS);
    ESP.restart();
  }
}

void loop() {
  updateSignaling();
}
