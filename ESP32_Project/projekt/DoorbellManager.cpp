#include "DoorbellManager.h"

DoorbellManager::DoorbellManager() {
    lastState = LOW;
    lastDebounceTime = 0;
    debounceDelay = 50;
}

void DoorbellManager::init() {
    pinMode(PIN_DOORBELL, INPUT); 
}

bool DoorbellManager::isRinging() {
    bool currentState = digitalRead(PIN_DOORBELL);
    bool triggered = false;

    if (currentState == HIGH && lastState == LOW) {
        if ((millis() - lastDebounceTime) > debounceDelay) {
            triggered = true;
            lastDebounceTime = millis();
        }
    }
    
    if (currentState != lastState) {
        lastDebounceTime = millis();
    }
    
    lastState = currentState;
    return triggered;
}