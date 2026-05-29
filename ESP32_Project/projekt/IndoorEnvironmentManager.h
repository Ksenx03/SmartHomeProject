#ifndef INDOORENVIRONMENTMANAGER_H
#define INDOORENVIRONMENTMANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <AHTxx.h>           // Niezawodna biblioteka
#include <Adafruit_BMP280.h> // Biblioteka do ciśnienia
#include <ArduinoJson.h>

class IndoorEnvironmentManager {
private:
    AHTxx aht20;         // Zmieniona nazwa, dokladnie jak w dzialajacym kodzie V2
    Adafruit_BMP280 bmp; // Obiekt czujnika BMP280 (ciśnienie)
    
    unsigned long lastReadTime;
    const unsigned long readInterval = 5000; // Interwał odczytu: 5 sekund
    
    float currentTemp;
    float currentHum;
    float currentPress;
    
    bool isInitialized;
    bool ahtReady; // Flaga poprawnego działania AHT20
    bool bmpReady; // Flaga poprawnego działania BMP280

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