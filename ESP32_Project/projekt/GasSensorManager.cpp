#include "GasSensorManager.h"

GasSensorManager::GasSensorManager() {
    lastReadTime = 0;
    rawValue = 0;
    gasPercentage = 0;
    isAlarm = false;
}

void GasSensorManager::init() {
    analogReadResolution(12);
}

bool GasSensorManager::loop() {
    if (millis() - lastReadTime >= readInterval) {
        lastReadTime = millis();

        rawValue = analogRead(PIN_MQ2);
        gasPercentage = constrain(map(rawValue, 0, 4095, 0, 100), 0, 100);
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

bool GasSensorManager::getAlarmState() { return isAlarm; }
bool GasSensorManager::isAlarmActive() { return isAlarm; }
int GasSensorManager::getRawValue() { return rawValue; }
int GasSensorManager::getPercentage() { return gasPercentage; }