#include "signaling.h"

#include <Arduino.h>

#include "config.h"

namespace {
const unsigned long SIGNAL_DURATION_MS = 500;
const unsigned int SUCCESS_TONE_HZ = 2000;
const unsigned int FAILURE_TONE_HZ = 1000;

bool signalActive = false;
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
  // aciona o LED e o som correspondentes ao resultado
  digitalWrite(LED_GREEN_PIN, success ? HIGH : LOW);
  digitalWrite(LED_RED_PIN, success ? LOW : HIGH);
  tone(BUZZER_PIN, success ? SUCCESS_TONE_HZ : FAILURE_TONE_HZ);

  // marca o início do intervalo de sinalização
  signalStartedAt = millis();
  signalActive = true;
}

void updateSignaling() {
  // desliga as saídas ao fim do intervalo sem bloquear o loop
  if (signalActive && millis() - signalStartedAt >= SIGNAL_DURATION_MS) {
    digitalWrite(LED_GREEN_PIN, LOW);
    digitalWrite(LED_RED_PIN, LOW);
    noTone(BUZZER_PIN);
    signalActive = false;
  }
}
