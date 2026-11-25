#ifndef ENVIRONMENTMANAGER_H
#define ENVIRONMENTMANAGER_H

#include <Arduino.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define PIN_DHT 32
#define DHT_TYPE DHT11

class EnvironmentManager {
private:
    DHT dht;
    unsigned long lastReadTime;
    const unsigned long readInterval = 5000;
    
    float currentTemp;
    float currentHum;

public:
    EnvironmentManager();
    void init();
    bool loop(); 
    String getSensorJson(); 
    float getTemp(); 
    float getHum();
};

#endif