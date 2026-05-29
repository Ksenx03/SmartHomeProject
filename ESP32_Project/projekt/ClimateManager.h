#ifndef CLIMATE_MANAGER_H
#define CLIMATE_MANAGER_H

#include <Arduino.h>

class ClimateManager {
private:
    const uint8_t pinPeltier = 16;
    const uint8_t pinFan = 17;
    
    bool peltierState;
    bool fanState;

public:
    ClimateManager();
    void init();
    
    void setPeltier(bool state);
    void setFan(bool state);
    void togglePeltier();
    void toggleFan();
    
    bool getPeltierState() const;
    bool getFanState() const;
    
    // Metoda pomocnicza do parsowania komend tekstowych (np. z MQTT)
    void processCommand(const String& target, const String& stateCmd);
};

#endif // CLIMATE_MANAGER_H