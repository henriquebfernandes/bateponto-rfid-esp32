#include <Arduino.h>

#include "clock/clock_sync.h"
#include "config.h"
#include "signaling/signaling.h"
#include "web/web_server.h"
#include "wifi/wifi_manager.h"

namespace {
constexpr unsigned long RESTART_DELAY_MS = 5000;
bool setup_success = false;
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);

  // interrompe a inicialização na primeira falha e sinaliza o resultado
  setup_success = beginSignaling() && connectWiFi() && syncClock() &&
                  beginWebServer();
  signalResult(setup_success);

  // aguarda antes de reiniciar quando a inicialização falha
  if (!setup_success) {
    Serial.println("Setup falhou. Reiniciando...");
    delay(RESTART_DELAY_MS);
    ESP.restart();
  }
}

void loop() {
  // atualiza a sinalização e atende requisições a cada ciclo
  updateSignaling();
  updateWebServer();
}
