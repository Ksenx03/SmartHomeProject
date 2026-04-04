#ifndef RFIDMANAGER_H
#define RFIDMANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>

// Piny zgodnie z CSV + dodany pin RST
#define PIN_RFID_SS 5
#define PIN_RFID_RST 4

class RfidManager {
private:
    MFRC522 mfrc522;
    String lastReadUID;
    
    // Funkcja pomocnicza zamieniająca bajty na czytelny tekst (HEX)
    String getUIDString(byte *buffer, byte bufferSize);

public:
    RfidManager();
    void init();
    
    // Funkcja zwraca true, jeśli przyłożono NOWĄ kartę
    bool loop();
    
    // Generuje paczkę JSON z numerem karty
    String getSensorJson();
    
    // Zwykły getter do pobrania samego numeru w kodzie
    String getUID();
};

#endif