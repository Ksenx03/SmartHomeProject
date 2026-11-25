#include "RfidManager.h"

RfidManager::RfidManager() : mfrc522(PIN_RFID_SS, PIN_RFID_RST) {
    lastReadUID = "";
    lastCheckTime = 0;
    lastPollResult = false;
}

void RfidManager::init() {
    SPI.begin(18, 19, 23, PIN_RFID_SS);
    mfrc522.PCD_Init();
    delay(50);
    mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

    byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    Serial.print("[RFID] Wersja ukladu MFRC522: 0x");
    Serial.println(version, HEX);
    if (version == 0x00 || version == 0xFF) {
        Serial.println("[RFID] BLAD KRYTYCZNY: Brak komunikacji SPI z czytnikiem RC522! Sprawdz przewody.");
    } else {
        Serial.println("[RFID] Czytnik RC522 gotowy do pracy!");
    }
}

String RfidManager::getUIDString(byte *buffer, byte bufferSize) {
    String uid = "";
    for (byte i = 0; i < bufferSize; i++) {
        uid += String(buffer[i] < 0x10 ? "0" : "");
        uid += String(buffer[i], HEX);
    }
    uid.toUpperCase();
    return uid;
}

bool RfidManager::loop() {
    unsigned long now = millis();
    
    // Nie odpytujemy magistrali SPI częściej niż co 80 ms!
    // Pomiędzy pomiarami zwracamyfalse, żeby główna pętla nie wysyłała ponownie MQTT,
    // a licznik trzymania karty w projekt.ino (1200ms tolerancji) bez problemu utrzyma stan.
    if (now - lastCheckTime < pollInterval) {
        return false;
    }
    lastCheckTime = now;

    byte bufferATQA[2];
    byte bufferSize = sizeof(bufferATQA);

    // Pojedyncze zapytanie WakeupA budzi zarówno nowe karty, jak i te w stanie HALT
    MFRC522::StatusCode status = mfrc522.PICC_WakeupA(bufferATQA, &bufferSize);

    if (status != MFRC522::STATUS_OK && status != MFRC522::STATUS_COLLISION) {
        return false;
    }

    if (!mfrc522.PICC_ReadCardSerial()) {
        return false;
    }

    lastReadUID = getUIDString(mfrc522.uid.uidByte, mfrc522.uid.size);
    mfrc522.PCD_StopCrypto1();
    return true;
}

String RfidManager::getSensorJson() {
    JsonDocument doc;
    doc["uid"] = lastReadUID;
    String output;
    serializeJson(doc, output);
    return output;
}

String RfidManager::getUID() {
    return lastReadUID;
}