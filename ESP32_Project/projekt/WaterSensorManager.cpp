#include "WaterSensorManager.h"

WaterSensorManager::WaterSensorManager() {
    lastReadTime = 0;
    rawValue = 0;
    waterLevelPercentage = 0;
    isAlarm = false;
}

void WaterSensorManager::init() {
    // Rozdzielczość 12-bitowa (0-4095)
    analogReadResolution(12);
}

bool WaterSensorManager::loop() {
    if (millis() - lastReadTime >= readInterval) {
        lastReadTime = millis();

        // 1. Odczyt napięcia z czujnika
        rawValue = analogRead(PIN_WATER);

        // 2. Skalowanie na procenty (0-100%)
        waterLevelPercentage = map(rawValue, 0, 4095, 0, 100);
        waterLevelPercentage = constrain(waterLevelPercentage, 0, 100);

        // 3. Weryfikacja, czy poziom wody przekroczył próg alarmowy
        isAlarm = (rawValue > WATER_THRESHOLD);

        return true; 
    }
    return false;
}

String WaterSensorManager::getSensorJson() {
    JsonDocument doc;
    doc["raw"] = rawValue;
    doc["percentage"] = waterLevelPercentage;
    doc["alarm"] = isAlarm ? "ON" : "OFF";

    String output;
    serializeJson(doc, output);
    return output;
}

bool WaterSensorManager::getAlarmState() {
    return isAlarm;
}

bool WaterSensorManager::isAlarmActive() {
    return isAlarm;
}