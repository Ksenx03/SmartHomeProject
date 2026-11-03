#ifndef CLIMATEMANAGER_H
#define CLIMATEMANAGER_H

#include <Arduino.h>

class ClimateManager {
private:
    int relayPin;
    bool isActive;
    float targetTemp;

public:
    ClimateManager(int pin);
    
    void init();
    void setActive(bool state);
    bool getIsActive();
    void setTargetTemp(float temp);
    
    // Zostawiamy tę metodę, bo używasz jej w projekt.ino do sterowania wentylatorem
    int update(float currentTemp); 
};

#endif