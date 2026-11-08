#ifndef CLIMATEMANAGER_H
#define CLIMATEMANAGER_H

#include <Arduino.h>

class ClimateManager {
private:
    int relayPin;
    bool isActive;       // Czy przekaźnik aktualnie grzeje
    bool autoMode;       // Czy termostat (tryb AUTO) jest aktywny
    float targetTemp;    // Temperatura zadana z aplikacji
    const float hysteresis = 0.5; // Histereza 0.5*C zapobiegająca ciągłemu klikaniu przekaźnika

    void setRelay(bool state);

public:
    ClimateManager(int pin);
    void init();
    void setActive(bool state);      // Ręczne ON / OFF
    void setAutoMode(bool enabled);  // Włączenie trybu AUTO
    bool getIsActive();
    bool isAutoMode();
    void setTargetTemp(float temp);  // Ustawienie temperatury (np. Eco, Comfort, Boost, suwak)
    float getTargetTemp();
    void update(float currentTemp);  // Wywoływane w loop() - steruje termostatem
};

#endif