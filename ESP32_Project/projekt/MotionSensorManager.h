#ifndef MOTIONSENSORMANAGER_H
#define MOTIONSENSORMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define PIN_PIR 15

class MotionSensorManager {
private:
    bool isEnabled;
    bool motionDetected;
    unsigned long lastMotionTime;
    const unsigned long timeout = 10000;
    bool isLightControlledByPIR; 

public:
    MotionSensorManager();
    void init();
    bool loop();
    void processCommand(String jsonCommand);
    String getSensorJson();

    bool isMotion();
    bool getIsEnabled();
    bool checkTimeout();
    void setPirControlled(bool state);
    bool getPirControlled();
};

#endif