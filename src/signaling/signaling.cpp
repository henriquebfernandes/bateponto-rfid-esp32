#include "signaling.h"

#include <Arduino.h>

#include "config.h"

namespace {
const unsigned long SIGNAL_DURATION_MS = 500;
const unsigned long BEEP_DURATION_MS = 100;
const unsigned long BEEP_GAP_MS = 100;
const unsigned long SECOND_BEEP_START_MS = BEEP_DURATION_MS + BEEP_GAP_MS;

bool signalActive = false;
bool repeatBeep = false;
unsigned long signalStartedAt = 0;
}

bool beginSignaling() {
  // configura as saídas e deixa os LEDs e o buzzer desligados
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  return true;
}

void signalResult(bool success) {
  // acende o LED correspondente e inicia o primeiro bip
  digitalWrite(LED_GREEN_PIN, success ? HIGH : LOW);
  digitalWrite(LED_RED_PIN, success ? LOW : HIGH);
  digitalWrite(BUZZER_PIN, HIGH);

  // programa o segundo bip apenas em caso de falha
  repeatBeep = !success;
  signalStartedAt = millis();
  signalActive = true;
}

void updateSignaling() {
  if (!signalActive) {
    return;
  }

  const unsigned long elapsed = millis() - signalStartedAt;

  // mantém o buzzer ligado apenas durante os bipes
  const bool buzzerOn = elapsed < BEEP_DURATION_MS ||
                        (repeatBeep && elapsed >= SECOND_BEEP_START_MS &&
                         elapsed < SECOND_BEEP_START_MS + BEEP_DURATION_MS);
  digitalWrite(BUZZER_PIN, buzzerOn ? HIGH : LOW);

  // desliga as saídas ao fim do intervalo sem bloquear o loop
  if (elapsed >= SIGNAL_DURATION_MS) {
    digitalWrite(LED_GREEN_PIN, LOW);
    digitalWrite(LED_RED_PIN, LOW);
    signalActive = false;
  }
}
