#ifndef DOORBELLMANAGER_H
#define DOORBELLMANAGER_H

#include <Arduino.h>

// Definicja pinu zgodnie ze schematem połączeń
#define PIN_DOORBELL 33 

class DoorbellManager {
private:
    bool lastState;               // Ostatni znany stan czujnika
    unsigned long lastDebounceTime; // Czas ostatniej zmiany (do debouncingu)
    unsigned long debounceDelay;  // Opóźnienie eliminujące drgania styków

public:
    DoorbellManager();
    void init();
    
    // Zwraca true tylko w momencie wykrycia dotknięcia (tzw. zbocze narastające)
    bool isRinging(); 
};

#endif