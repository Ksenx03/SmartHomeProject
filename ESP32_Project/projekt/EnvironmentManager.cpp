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
    // Sprawdzamy, czy minęło 5 sekund od ostatniego odczytu
    if (millis() - lastReadTime >= readInterval) {
        lastReadTime = millis();

        float h = dht.readHumidity();
        float t = dht.readTemperature();

        // DHT czasem "gubi" ramkę danych. Zabezpieczamy się przed wartościami NaN (Not a Number)
        if (isnan(h) || isnan(t)) {
            Serial.println("Blad odczytu z czujnika DHT11!");
            return false;
        }

        currentTemp = t;
        currentHum = h;
        
        // Serial.print("Nowy odczyt: Temp = ");
        // Serial.print(currentTemp);
        // Serial.print("C, Wilg = ");
        // Serial.print(currentHum);
        // Serial.println("%");
        
        return true; // Sygnalizujemy, że są nowe, poprawne dane
    }
    return false;
}

String EnvironmentManager::getSensorJson() {
    JsonDocument doc;
    doc["temperature"] = currentTemp;
    doc["humidity"] = currentHum;

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