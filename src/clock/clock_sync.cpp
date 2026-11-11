#include "clock_sync.h"

#include <Arduino.h>
#include <esp_sntp.h>

#include "config.h"

bool syncClock() {
  Serial.println("Sincronizando o relógio por NTP...");
  
  // aplica o fuso horário e inicia a sincronização por NTP
  configTzTime(TIME_ZONE, NTP_SERVER);

  // aguarda a sincronização até atingir o tempo limite
  const unsigned long startedAt = millis();
  while (sntp_get_sync_status() != true) {
    if (millis() - startedAt >= NTP_TIMEOUT_MS) {
      Serial.println("Tempo limite para sincronizar o relógio por NTP.");
      return false;
    }
    delay(100);
  }

  // converte e exibe a data e a hora locais no monitor serial
  const time_t now = time(nullptr);
  struct tm localTime;
  localtime_r(&now, &localTime);

  char dateTime[32];
  strftime(dateTime, sizeof(dateTime), "%d/%m/%Y %H:%M:%S", &localTime);
  Serial.print("Data e hora local: ");
  Serial.println(dateTime);
  return true;
}
