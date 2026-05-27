#include "LightSensorManager.h"

LightSensorManager::LightSensorManager() {
    lastReadTime = 0;
    currentLux = 0.0;
}

void LightSensorManager::init() {
    Wire.begin(); 
    
    // Stabilizacja magistrali I2C przed pierwszą komunikacją
    delay(100);
    
    Serial.println("[BH1750] Proba inicjalizacji czujnika swiatla...");
    
    // ZMIANA: Użycie trybu MODE_2, który jest kompatybilny z tanimi klonami z Aliexpress
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2)) {
        Serial.println("[BH1750] SUKCES: Czujnik BH1750 zainicjowany (Tryb: MODE_2)!");
    } else {
        Serial.println("[BH1750] BLAD KRYTYCZNY: Nie wykryto BH1750!");
    }
}

bool LightSensorManager::loop() {
    // Sprawdzanie co 2 sekundy
    if (millis() - lastReadTime >= readInterval) {
        lastReadTime = millis();
        
        // Odczyt poziomu światła
        currentLux = lightMeter.readLightLevel();

        // Agresywne logowanie do terminala
        Serial.print("[BH1750] Aktualny odczyt z czujnika: ");
        Serial.print(currentLux);
        Serial.println(" lx");

        // Walidacja błędów biblioteki (wartości ujemne)
        if (currentLux < 0) {
            Serial.println("[BH1750] BLAD: Zwrocono wartosc ujemna.");
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