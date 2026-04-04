#include "MotionSensorManager.h"

MotionSensorManager::MotionSensorManager() {
    isEnabled = true; // Domyślnie PIR czuwa
    motionDetected = false;
    lastMotionTime = 0;
    isLightControlledByPIR = false;
}

void MotionSensorManager::init() {
    pinMode(PIN_PIR, INPUT_PULLDOWN);
}

bool MotionSensorManager::loop() {
    if (!isEnabled) return false;

    // Odczyt sprzętowy (HIGH = wykryto ruch)
    bool currentMotion = (digitalRead(PIN_PIR) == HIGH);

    if (currentMotion) {
        motionDetected = true;
        lastMotionTime = millis(); // Stoper resetuje się, gdy się ruszamy
    } else {
        motionDetected = false;
    }

    return motionDetected;
}

void MotionSensorManager::processCommand(String jsonCommand) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonCommand);
    if (error) return;

    // Komenda blokady czujnika: np. {"target": "pir", "state": "OFF"}
    if (doc["target"] == "pir" && doc.containsKey("state")) {
        String stateStr = doc["state"].as<String>();
        stateStr.toUpperCase();
        
        if (stateStr == "ON") {
            isEnabled = true;
            Serial.println("Czujnik PIR WŁĄCZONY");
        } else if (stateStr == "OFF") {
            isEnabled = false;
            Serial.println("Czujnik PIR WYŁĄCZONY");
        }
    }
}

bool MotionSensorManager::isMotion() {
    return motionDetected;
}

bool MotionSensorManager::getIsEnabled() {
    return isEnabled;
}

bool MotionSensorManager::checkTimeout() {
    // Odliczamy czas TYLKO jeśli światło włączył PIR i w pokoju nie ma ruchu
    if (isLightControlledByPIR && !motionDetected) {
        if (millis() - lastMotionTime >= timeout) {
            return true; // Czas minął!
        }
    }
    return false;
}

void MotionSensorManager::setPirControlled(bool state) {
    isLightControlledByPIR = state;
    if (state) {
        lastMotionTime = millis(); // Zaczynamy odliczać timer 10s
    }
}

bool MotionSensorManager::getPirControlled() {
    return isLightControlledByPIR;
}

String MotionSensorManager::getSensorJson() {
    JsonDocument doc;
    doc["motion"] = motionDetected ? "YES" : "NO";
    doc["enabled"] = isEnabled ? "ON" : "OFF";
    doc["pir_controls_light"] = isLightControlledByPIR ? "YES" : "NO";
    
    String output;
    serializeJson(doc, output);
    return output;
}