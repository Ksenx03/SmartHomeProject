#include "GasSensorManager.h"

GasSensorManager::GasSensorManager() {
    lastReadTime = 0;
    rawValue = 0;
    gasPercentage = 0;
    isAlarm = false;
}

void GasSensorManager::init() {
    // Ustawiamy rozdzielczość odczytu analogowego na 12 bitów (standard w ESP32, ale warto wymusić dla czytelności)
    analogReadResolution(12);
    
    // Piny od 34 do 39 w ESP32 to tzw. piny "Input Only", 
    // więc nie musimy używać funkcji pinMode(PIN_MQ2, INPUT).
}

bool GasSensorManager::loop() {
    if (millis() - lastReadTime >= readInterval) {
        lastReadTime = millis();

        // 1. Fizyczny odczyt napięcia z pinu ADC
        rawValue = analogRead(PIN_MQ2);

        // 2. Skalowanie do wartości procentowej dla aplikacji mobilnej
        gasPercentage = map(rawValue, 0, 4095, 0, 100);
        // Zabezpieczenie przed wyjściem poza ramy 0-100%
        gasPercentage = constrain(gasPercentage, 0, 100);

        // 3. Sprawdzenie, czy przekroczono próg alarmowy
        isAlarm = (rawValue > GAS_THRESHOLD);

        return true; 
    }
    return false;
}

String GasSensorManager::getSensorJson() {
    JsonDocument doc;
    doc["raw"] = rawValue;
    doc["percentage"] = gasPercentage;
    doc["alarm"] = isAlarm ? "ON" : "OFF";

    String output;
    serializeJson(doc, output);
    return output;
}

bool GasSensorManager::getAlarmState() {
    return isAlarm;
}

bool GasSensorManager::isAlarmActive() {
    return isAlarm;
}