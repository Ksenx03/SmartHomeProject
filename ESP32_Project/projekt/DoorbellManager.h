#ifndef DOORBELLMANAGER_H
#define DOORBELLMANAGER_H

#include <Arduino.h>

#define PIN_DOORBELL 33 

class DoorbellManager {
private:
    bool lastState;
    unsigned long lastDebounceTime;
    unsigned long debounceDelay;

public:
    DoorbellManager();
    void init();
    bool isRinging(); 
};

#endif