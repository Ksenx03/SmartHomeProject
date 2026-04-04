#include "DoorbellManager.h"

DoorbellManager::DoorbellManager() {
    lastState = LOW;
    lastDebounceTime = 0;
    debounceDelay = 50; // 50 ms na wyeliminowanie fałszywych odczytów
}

void DoorbellManager::init() {
    // TTP223 wysyła stan wysoki (HIGH) w momencie dotknięcia
    pinMode(PIN_DOORBELL, INPUT); 
}

bool DoorbellManager::isRinging() {
    bool currentState = digitalRead(PIN_DOORBELL);
    bool triggered = false;

    // Reagujemy tylko na zmianę ze stanu LOW na HIGH (zbocze narastające)
    if (currentState == HIGH && lastState == LOW) {
        if ((millis() - lastDebounceTime) > debounceDelay) {
            triggered = true;
            lastDebounceTime = millis();
        }
    }
    
    // Zabezpieczenie przed podwójnym odczytem w przypadku "drgania"
    if (currentState != lastState) {
        lastDebounceTime = millis();
    }
    
    lastState = currentState;
    return triggered;
}