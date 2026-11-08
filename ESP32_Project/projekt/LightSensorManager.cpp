#include "LightSensorManager.h"

LightSensorManager::LightSensorManager() {
    lastReadTime = 0;
    currentLux = 250.0; // Bezpieczna wartosc domyslna (DZIEN), gdyby czujnik byl odlaczony
    isInitialized = false;
}

bool LightSensorManager::tryInitialize() {
    // 1. Proba na domyslnym adresie 0x23 (pin ADDR wolny lub do GND)
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2, 0x23, &Wire)) {
        Serial.println("[BH1750] SUKCES: Czujnik zainicjowany pod adresem 0x23!");
        return true;
    }
    
    // 2. Proba na alternatywnym adresie 0x5C (pin ADDR do VCC)
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2, 0x5C, &Wire)) {
        Serial.println("[BH1750] SUKCES: Czujnik zainicjowany pod adresem 0x5C!");
        return true;
    }

    return false;
}

void LightSensorManager::init() {
    Wire.begin();
    delay(100);
    
    Serial.println("[BH1750] Proba inicjalizacji czujnika swiatla...");
    isInitialized = tryInitialize();
    
    if (!isInitialized) {
        Serial.println("[BH1750] BLAD: Nie wykryto BH1750 na magistrali I2C (sprawdz przewody SDA/SCL)!");
    }
}

bool LightSensorManager::loop() {
    if (millis() - lastReadTime >= readInterval) {
        lastReadTime = millis();

        // Jesli czujnik nie byl zainicjowany przy starcie (np. luzny styk), sprobuj ponownie po cichu
        if (!isInitialized) {
            isInitialized = tryInitialize();
            if (!isInitialized) {
                return false; // Nie wywolujemy readLightLevel(), wiec biblioteka nie sypie bledami!
            }
        }

        float measuredLux = lightMeter.readLightLevel();
        if (measuredLux < 0) {
            Serial.println("[BH1750] Ostrzezenie: Utracono polaczenie z czujnikiem (ponowna inicjalizacja przy kolejnym cyklu).");
            isInitialized = false; // Wymusi ponowna konfiguracje za 2 sekundy
            return false;
        }

        currentLux = measuredLux;
        return true;
    }
    return false;
}

String LightSensorManager::getSensorJson() {
    JsonDocument doc;
    doc["lux"] = currentLux;
    String output;
    serializeJson(doc, output);
    return output;
}

float LightSensorManager::getLux() {
    return currentLux;
}