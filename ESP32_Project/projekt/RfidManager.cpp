#include "RfidManager.h"

// Inicjalizacja obiektu biblioteki z naszymi pinami
RfidManager::RfidManager() : mfrc522(PIN_RFID_SS, PIN_RFID_RST) {
    lastReadUID = "";
}

void RfidManager::init() {
    SPI.begin();             // Uruchomienie magistrali SPI
    mfrc522.PCD_Init();      // Inicjalizacja samego czytnika
    // Delikatne opóźnienie, by układ RC522 zdążył wystartować
    delay(4);
    Serial.println("Czytnik RFID RC522 gotowy do pracy!");
}

String RfidManager::getUIDString(byte *buffer, byte bufferSize) {
    String uid = "";
    for (byte i = 0; i < bufferSize; i++) {
        uid += String(buffer[i] < 0x10 ? "0" : ""); // Dodanie zera wiodącego
        uid += String(buffer[i], HEX);
    }
    uid.toUpperCase();
    return uid;
}

bool RfidManager::loop() {
    // OBEJŚCIE SPRZĘTOWE DLA CIĄGŁEGO ODCZYTU (WUPA zamiast REQA)
    // Tworzymy mały bufor wymagany przez funkcję wybudzającą
    byte bufferATQA[2];
    byte bufferSize = sizeof(bufferATQA);
    
    // Używamy PICC_WakeupA, które potrafi wybudzić kartę ze stanu uśpienia (HaltA)
    // Zastępuje to wadliwe PICC_IsNewCardPresent() w przypadku przytrzymywania kart
    mfrc522.PICC_WakeupA(bufferATQA, &bufferSize);
    
    // Próbujemy odczytać numer seryjny wybudzonej karty
    if (!mfrc522.PICC_ReadCardSerial()) {
        return false; // Karty fizycznie nie ma w zasięgu czytnika
    }

    // Mamy kartę! Zapisujemy odczyt do zmiennej globalnej klasy
    lastReadUID = getUIDString(mfrc522.uid.uidByte, mfrc522.uid.size);
    
    // Usypiamy kartę, aby zresetować jej stan (wymóg magistrali SPI), 
    // w kolejnym kroku WakeupA znów ją obudzi.
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    return true; 
}

// bool RfidManager::loop() {
//     // 1. Sprawdzamy, czy w pobliżu czytnika w ogóle jest jakaś karta
//     if (!mfrc522.PICC_IsNewCardPresent()) {
//         return false;
//     }

//     // 2. Jeśli jest, próbujemy odczytać jej numer seryjny (UID)
//     if (!mfrc522.PICC_ReadCardSerial()) {
//         return false;
//     }

//     // 3. Konwertujemy surowe bajty na czytelny ciąg znaków (np. "A1B2C3D4")
//     lastReadUID = getUIDString(mfrc522.uid.uidByte, mfrc522.uid.size);
    
//     // Usypiamy kartę po odczycie (dobra praktyka w RFID)
//     mfrc522.PICC_HaltA();

//     return true; 
// }

// bool RfidManager::loop() {
//     // 1. Sprawdzamy obecność karty
//     if (!mfrc522.PICC_IsNewCardPresent()) return false;
    
//     // 2. Czytamy numer seryjny
//     if (!mfrc522.PICC_ReadCardSerial()) return false;

//     // 3. Zapisujemy odczyt
//     lastReadUID = getUIDString(mfrc522.uid.uidByte, mfrc522.uid.size);
    
//     // MUSIMY uśpić kartę, żeby za milisekundę IsNewCardPresent znów zadziałało!
//     mfrc522.PICC_HaltA();
//     mfrc522.PCD_StopCrypto1();

//     return true; 
// }

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