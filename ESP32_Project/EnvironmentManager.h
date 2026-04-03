#ifndef ENVIRONMENTMANAGER_H
#define ENVIRONMENTMANAGER_H

#include <Arduino.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define PIN_DHT 32      // Zmień na pin, pod który fizycznie podpiąłeś czujnik
#define DHT_TYPE DHT11

class EnvironmentManager {
private:
    DHT dht;
    unsigned long lastReadTime;
    const unsigned long readInterval = 5000; // Interwał odczytu: 5 sekund (5000 ms)
    
    float currentTemp;
    float currentHum;

public:
    EnvironmentManager();
    void init();
    
    // Zwraca true, jeśli nadszedł czas i udało się odczytać nowe dane
    bool loop(); 
    
    // Generuje paczkę JSON z wynikami
    String getSensorJson(); 
    
    // Zwykły getter do przyszłej histerezy wentylatora
    float getTemp(); 
};

#endif