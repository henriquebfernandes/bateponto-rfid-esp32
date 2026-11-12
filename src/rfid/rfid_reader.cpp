#include "rfid_reader.h"

#include <Arduino.h>
#include <MFRC522.h>
#include <SPI.h>

#include "config.h"

namespace {
MFRC522 reader(RFID_SS_PIN, RFID_RST_PIN);
}

bool beginRfidReader() {
  SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);
  reader.PCD_Init();

  // verifica se o leitor responde no barramento SPI
  const byte version = reader.PCD_ReadRegister(MFRC522::VersionReg);
  if (version == 0x00 || version == 0xFF) {
    Serial.println("Falha na comunicação com o leitor RC522.");
    return false;
  }

  return true;
}

bool readRfidCard(String& uid) {
  if (!reader.PICC_IsNewCardPresent() || !reader.PICC_ReadCardSerial()) {
    return false;
  }

  // evita leituras repetidas enquanto o cartão permanece no campo do leitor
  reader.PICC_HaltA();

  constexpr char HEX_DIGITS[] = "0123456789ABCDEF";
  char encodedUid[2 * sizeof(reader.uid.uidByte) + 1];
  for (byte index = 0; index < reader.uid.size; ++index) {
    const byte value = reader.uid.uidByte[index];
    encodedUid[2 * index] = HEX_DIGITS[value >> 4];
    encodedUid[2 * index + 1] = HEX_DIGITS[value & 0x0F];
  }
  encodedUid[2 * reader.uid.size] = '\0';
  uid = encodedUid;
  Serial.printf("UID: %s\n", encodedUid);
  return true;
}
