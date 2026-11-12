#include <Arduino.h>

#include "attendance/attendance_storage.h"
#include "attendance/rfid_attendance.h"
#include "cards/card_enrollment.h"
#include "cards/card_storage.h"
#include "clock/clock_sync.h"
#include "config.h"
#include "people/people_storage.h"
#include "rfid/rfid_reader.h"
#include "signaling/signaling.h"
#include "web/web_server.h"
#include "wifi/wifi_manager.h"

namespace {
constexpr unsigned long RESTART_DELAY_MS = 5000;
bool setupSucceeded = false;
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);

  // para na primeira falha e sinaliza o resultado
  setupSucceeded = beginSignaling() &&
                   connectWiFi() &&
                   syncClock() &&
                   beginPeopleStorage() &&
                   beginCardStorage() &&
                   beginAttendanceStorage() &&
                   beginWebServer() &&
                   beginRfidReader();
  signalResult(setupSucceeded);

  if (!setupSucceeded) {
    Serial.println("Falha na inicialização. Aguardando reinício.");
  }
}

void loop() {
  // atualiza a sinalização mesmo enquanto aguarda o reinício
  updateSignaling();
  if (!setupSucceeded) {
    // começa a contar na primeira passagem pelo loop após a falha
    static const unsigned long restartWaitStartedAt = millis();
    if (millis() - restartWaitStartedAt >= RESTART_DELAY_MS) {
      ESP.restart();
    }
    delay(1);
    return;
  }

  updateCardEnrollment();
  updateWebServer();

  // encaminha a leitura ao cadastro ativo ou ao registro de ponto
  String uid;
  if (readRfidCard(uid) && !consumeEnrollmentCard(uid)) {
    signalResult(registerRfidAttendance(uid));
  }
}
