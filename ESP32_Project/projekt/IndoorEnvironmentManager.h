#ifndef INDOORENVIRONMENTMANAGER_H
#define INDOORENVIRONMENTMANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <AHTxx.h>
#include <Adafruit_BMP280.h>
#include <ArduinoJson.h>

class IndoorEnvironmentManager {
private:
    AHTxx aht20;
    Adafruit_BMP280 bmp;
    
    unsigned long lastReadTime;
    const unsigned long readInterval = 5000;
    
    float currentTemp;
    float currentHum;
    float currentPress;
    
    bool isInitialized;
    bool ahtReady;
    bool bmpReady;

public:
    IndoorEnvironmentManager();
    void init();
    bool loop(); 
    String getSensorJson(); 
    
    float getTemp(); 
    float getHum();
    float getPress();
};

#endif