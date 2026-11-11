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

void updateRfidReader() {
  if (!reader.PICC_IsNewCardPresent() || !reader.PICC_ReadCardSerial()) {
    return;
  }

  // evita leituras repetidas enquanto o cartão permanece no campo do leitor
  reader.PICC_HaltA();

  Serial.print("UID: ");
  for (byte index = 0; index < reader.uid.size; ++index) {
    Serial.printf("%02X", static_cast<unsigned int>(reader.uid.uidByte[index]));
  }
  Serial.println();
}
