#ifndef LIGHTSENSORMANAGER_H
#define LIGHTSENSORMANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <ArduinoJson.h>

class LightSensorManager {
private:
    BH1750 lightMeter;
    unsigned long lastReadTime;
    const unsigned long readInterval = 2000;
    float currentLux;
    bool isInitialized; // Sprawdza, czy czujnik zostal poprawnie skonfigurowany

    bool tryInitialize();

public:
    LightSensorManager();
    void init();
    bool loop();
    String getSensorJson();
    float getLux();
    bool isReady() const { return isInitialized; }
};

#endif