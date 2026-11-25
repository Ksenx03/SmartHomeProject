#include "EnvironmentManager.h"

EnvironmentManager::EnvironmentManager() : dht(PIN_DHT, DHT_TYPE) {
    lastReadTime = 0;
    currentTemp = 0.0;
    currentHum = 0.0;
}

void EnvironmentManager::init() {
    dht.begin();
}

bool EnvironmentManager::loop() {
    if (millis() - lastReadTime >= readInterval) {
        lastReadTime = millis();

        float h = dht.readHumidity();
        float t = dht.readTemperature();

        if (isnan(h) || isnan(t)) {
            Serial.println("Blad odczytu z czujnika DHT11!");
            return false;
        }

        currentTemp = t;
        currentHum = h;
        return true;
    }
    return false;
}

String EnvironmentManager::getSensorJson() {
    JsonDocument doc;
    JsonObject outdoor = doc["outdoor"].to<JsonObject>();
    outdoor["temperature"] = currentTemp;
    outdoor["humidity"] = currentHum;

    String output;
    serializeJson(doc, output);
    return output;
}

float EnvironmentManager::getTemp() {
    return currentTemp;
}

float EnvironmentManager::getHum() {
    return currentHum;
}