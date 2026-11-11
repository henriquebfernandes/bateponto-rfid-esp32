#pragma once

//Configuração da Serial
const unsigned long SERIAL_BAUD_RATE = 115200;

//Configuração do Wi-Fi
const char WIFI_SSID[] = "nome-da-rede";
const char WIFI_PASSWORD[] = "senha-da-rede";
const unsigned long WIFI_TIMEOUT_MS = 20000;

// Configuração do relógio
const char NTP_SERVER[] = "pool.ntp.org";
const char TIME_ZONE[] = "BRT3";  // Horário de Brasília (UTC-3), no formato POSIX.
const unsigned long NTP_TIMEOUT_MS = 10000;

//Configuração dos Pins
const int LED_GREEN_PIN = 2;
const int LED_RED_PIN = 4;
const int BUZZER_PIN = 15;
