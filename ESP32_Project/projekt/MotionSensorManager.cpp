#include "MotionSensorManager.h"

MotionSensorManager::MotionSensorManager() {
    isEnabled = true;
    motionDetected = false;
    lastMotionTime = 0;
    isLightControlledByPIR = false;
}

void MotionSensorManager::init() {
    pinMode(PIN_PIR, INPUT_PULLDOWN);
}

bool MotionSensorManager::loop() {
    if (!isEnabled) return false;

    bool currentMotion = (digitalRead(PIN_PIR) == HIGH);

    if (currentMotion) {
        motionDetected = true;
        lastMotionTime = millis();
    } else {
        motionDetected = false;
    }

    return motionDetected;
}

void MotionSensorManager::processCommand(String jsonCommand) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonCommand);
    if (error) return;

    if (doc["target"] == "pir" && doc.containsKey("state")) {
        String stateStr = doc["state"].as<String>();
        stateStr.toUpperCase();
        
        if (stateStr == "ON") {
            isEnabled = true;
            Serial.println("Czujnik PIR WLACZONY");
        } else if (stateStr == "OFF") {
            isEnabled = false;
            Serial.println("Czujnik PIR WYLACZONY");
        }
    }
}

bool MotionSensorManager::isMotion() { return motionDetected; }
bool MotionSensorManager::getIsEnabled() { return isEnabled; }

bool MotionSensorManager::checkTimeout() {
    if (isLightControlledByPIR && !motionDetected) {
        if (millis() - lastMotionTime >= timeout) {
            return true;
        }
    }
    return false;
}

void MotionSensorManager::setPirControlled(bool state) {
    isLightControlledByPIR = state;
    if (state) {
        lastMotionTime = millis();
    }
}

bool MotionSensorManager::getPirControlled() { return isLightControlledByPIR; }

String MotionSensorManager::getSensorJson() {
    JsonDocument doc;
    doc["motion"] = motionDetected ? "YES" : "NO";
    doc["enabled"] = isEnabled ? "ON" : "OFF";
    doc["pir_controls_light"] = isLightControlledByPIR ? "YES" : "NO";
    
    String output;
    serializeJson(doc, output);
    return output;
}