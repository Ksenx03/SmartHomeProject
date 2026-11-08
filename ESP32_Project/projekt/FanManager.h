#ifndef FANMANAGER_H
#define FANMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define PIN_FAN_PWM 26
#define PWM_FREQ 25000
#define PWM_RES 8

class FanManager {
private:
    bool isOn;
    uint8_t currentSpeed;
    void updateFan();

public:
    FanManager();
    void init();
    void processCommand(String jsonCommand);
    int getSpeed();
    void setSpeed(int speed);
};

#endif