#pragma once

// configura a comunicação com o monitor serial
const unsigned long SERIAL_BAUD_RATE = 115200;

// configura as credenciais e o tempo limite da conexão Wi-Fi
const char WIFI_SSID[] = "SEU_SSID";
const char WIFI_PASSWORD[] = "SUA_SENHA";
const unsigned long WIFI_TIMEOUT_MS = 20000;

// configura o servidor NTP, o fuso horário e o tempo limite da sincronização
const char NTP_SERVER[] = "pool.ntp.org";
// define o fuso no formato POSIX, com BRT3 para o horário de Brasília
const char TIME_ZONE[] = "BRT3";
const unsigned long NTP_TIMEOUT_MS = 10000;

// configura o endereço local, a senha e a duração da sessão
// define o nome local usado no endereço bateponto.local
const char MDNS_HOSTNAME[] = "bateponto";
const char WEB_PASSWORD[] = "troque-esta-senha";
const unsigned long WEB_SESSION_TIMEOUT_MS = 30 * 60 * 1000UL;

// define os pinos dos LEDs e do buzzer
const int LED_GREEN_PIN = 2;
const int LED_RED_PIN = 4;
const int BUZZER_PIN = 15;

// define os pinos do leitor RC522
const int RFID_SCK_PIN = 18;
const int RFID_MISO_PIN = 19;
const int RFID_MOSI_PIN = 23;
const int RFID_SS_PIN = 5;
const int RFID_RST_PIN = 22;

// define o prazo de cadastro e o intervalo mínimo entre pontos da mesma pessoa
const unsigned long CARD_ENROLLMENT_TIMEOUT_MS = 15000;
const unsigned long ATTENDANCE_COOLDOWN_MS = 10000;
