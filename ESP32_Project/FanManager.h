#ifndef FANMANAGER_H
#define FANMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define PIN_FAN_PWM 26
#define PWM_FREQ 25000 // 25 kHz - standard dla wentylatorów PC
#define PWM_RES 8      // 8-bitowa rozdzielczość (wartości 0-255)

class FanManager {
private:
    bool isOn;
    uint8_t currentSpeed; // Prędkość w procentach (0 - 100%)

    void updateFan();     // Aktualizacja fizycznego sygnału PWM

public:
    FanManager();
    void init();
    void processCommand(String jsonCommand);
};

#endif