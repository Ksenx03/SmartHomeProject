#include "LightSensorManager.h"

LightSensorManager::LightSensorManager() {
    lastReadTime = 0;
    currentLux = 0.0;
}

void LightSensorManager::init() {
    // Rozpoczęcie pracy magistrali I2C (Domyślnie piny: SDA=21, SCL=22)
    Wire.begin(); 
    
    // Inicjalizacja czujnika BH1750
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
        Serial.println("Czujnik BH1750 zainicjowany poprawnie!");
    } else {
        Serial.println("Blad inicjalizacji BH1750! Sprawdz kable SDA i SCL.");
    }
}

bool LightSensorManager::loop() {
    if (millis() - lastReadTime >= readInterval) {
        lastReadTime = millis();
        
        // Odczyt poziomu światła w Luksach
        currentLux = lightMeter.readLightLevel();

        // Jeśli czujnik jest odłączony, biblioteka często zwraca -1 lub -2
        if (currentLux < 0) {
            Serial.println("Blad odczytu z BH1750");
            return false;
        }

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