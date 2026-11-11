#include "clock_sync.h"

#include <Arduino.h>
#include <esp_sntp.h>
#include <time.h>

#include "config.h"

bool syncClock() {
  Serial.println("Sincronizando o relógio por NTP...");
  configTzTime(TIME_ZONE, NTP_SERVER);

  const unsigned long startedAt = millis();
  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
    if (millis() - startedAt >= NTP_TIMEOUT_MS) {
      Serial.println("Tempo limite para sincronizar o relógio por NTP.");
      return false;
    }
    delay(100);
  }

  const time_t now = time(nullptr);
  struct tm localTime;
  localtime_r(&now, &localTime);

  char dateTime[32];
  strftime(dateTime, sizeof(dateTime), "%d/%m/%Y %H:%M:%S", &localTime);
  Serial.print("Data e hora local: ");
  Serial.println(dateTime);
  return true;
}
