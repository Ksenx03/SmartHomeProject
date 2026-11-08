#ifndef RFIDMANAGER_H
#define RFIDMANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>

#define PIN_RFID_SS 5
#define PIN_RFID_RST 4

class RfidManager {
private:
    MFRC522 mfrc522;
    String lastReadUID;
    unsigned long lastCheckTime;
    bool lastPollResult;
    const unsigned long pollInterval = 80; // Odczyt co 80 ms - zero zacinania ESP32!

    String getUIDString(byte *buffer, byte bufferSize);

public:
    RfidManager();
    void init();
    bool loop();
    String getSensorJson();
    String getUID();
};

#endif