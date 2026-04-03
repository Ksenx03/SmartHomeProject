#ifndef LIGHTSENSORMANAGER_H
#define LIGHTSENSORMANAGER_H

#include <Arduino.h>
#include <Wire.h>     // Wbudowana biblioteka obsługująca protokół I2C
#include <BH1750.h>   // Biblioteka czujnika
#include <ArduinoJson.h>

class LightSensorManager {
private:
    BH1750 lightMeter;
    unsigned long lastReadTime;
    const unsigned long readInterval = 2000; // Interwał: co 2 sekundy

    float currentLux;

public:
    LightSensorManager();
    void init();
    bool loop();
    String getSensorJson();
    float getLux();
};

#endif