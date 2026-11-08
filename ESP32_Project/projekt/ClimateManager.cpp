#include "ClimateManager.h"

ClimateManager::ClimateManager(int pin) {
    relayPin = pin;
    isActive = false;
    autoMode = false;
    targetTemp = 22.0;
}

void ClimateManager::init() {
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);
}

void ClimateManager::setRelay(bool state) {
    if (isActive != state) {
        isActive = state;
        digitalWrite(relayPin, isActive ? HIGH : LOW);
        if (isActive) {
            Serial.println("[HVAC] Ogrzewanie ON - przekaznik zwarty");
        } else {
            Serial.println("[HVAC] Ogrzewanie OFF - przekaznik rozwarty");
        }
    }
}

void ClimateManager::setActive(bool state) {
    autoMode = false; // Ręczne przełączenie wyłącza tryb AUTO
    setRelay(state);
}

void ClimateManager::setAutoMode(bool enabled) {
    autoMode = enabled;
    Serial.printf("[HVAC] Tryb AUTO: %s (Cel: %.1f *C)\n", autoMode ? "WLACZONY" : "WYLACZONY", targetTemp);
    if (!autoMode) {
        setRelay(false);
    }
}

bool ClimateManager::getIsActive() {
    return isActive;
}

bool ClimateManager::isAutoMode() {
    return autoMode;
}

void ClimateManager::setTargetTemp(float temp) {
    targetTemp = temp;
    autoMode = true; // Zmiana temperatury docelowej (np. Boost / suwak) automatycznie aktywuje termostat
    Serial.printf("[HVAC] Nowa temperatura docelowa: %.1f *C (Tryb AUTO aktywny)\n", targetTemp);
}

float ClimateManager::getTargetTemp() {
    return targetTemp;
}

void ClimateManager::update(float currentTemp) {
    if (!autoMode || currentTemp <= 0.0) {
        return; // Jeśli tryb AUTO wyłączony lub czujnik jeszcze nie odczytał poprawnej temperatury
    }

    // Jeśli w pokoju jest zimniej niż cel (np. 22.6 < 25.0 przy Boost) -> WŁĄCZ GRZANIE
    if (currentTemp < (targetTemp - hysteresis)) {
        setRelay(true);
    } 
    // Jeśli osiągnięto temperaturę docelową -> WYŁĄCZ GRZANIE
    else if (currentTemp >= targetTemp) {
        setRelay(false);
    }
}