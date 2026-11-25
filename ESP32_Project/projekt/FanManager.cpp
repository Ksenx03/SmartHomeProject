#include "FanManager.h"

FanManager::FanManager() {
    isOn = false;
    currentSpeed = 50;
}

void FanManager::init() {
    ledcAttach(PIN_FAN_PWM, PWM_FREQ, PWM_RES);
    updateFan();
}

void FanManager::updateFan() {
    uint32_t duty = isOn ? map(currentSpeed, 0, 100, 0, 255) : 0;
    ledcWrite(PIN_FAN_PWM, duty);
}

void FanManager::processCommand(String jsonCommand) {
    JsonDocument doc;
    if (deserializeJson(doc, jsonCommand)) return;

    if (doc.containsKey("state")) isOn = (doc["state"] == "ON");
    if (doc.containsKey("speed")) currentSpeed = constrain(doc["speed"].as<int>(), 0, 100);
    
    updateFan();
}

int FanManager::getSpeed() {
    if (!isOn) return 0;
    return currentSpeed;
}

void FanManager::setSpeed(int speed) {
    currentSpeed = constrain(speed, 0, 100);
    isOn = (currentSpeed > 0);
    updateFan();
}