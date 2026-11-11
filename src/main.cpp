#include <Arduino.h>

#include "config.h"
#include "wifi/wifi_manager.h"

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  connectWiFi();
}

void loop() {}
