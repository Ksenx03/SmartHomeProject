#ifndef MOTIONSENSORMANAGER_H
#define MOTIONSENSORMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define PIN_PIR 15

class MotionSensorManager {
private:
    bool isEnabled;             // Flaga zasilania logiki (PIR ON/OFF)
    bool motionDetected;        // Aktualny stan czujnika
    unsigned long lastMotionTime;
    const unsigned long timeout = 10000; // 10 sekund do wyłączenia (możesz zmienić)
    
    // Kluczowa flaga: Czy światło zostało włączone przez PIR?
    bool isLightControlledByPIR; 

public:
    MotionSensorManager();
    void init();
    bool loop(); // Odczyt sprzętowy pinu
    void processCommand(String jsonCommand); // Odbieranie komend MQTT
    String getSensorJson();

    bool isMotion();
    bool getIsEnabled();
    
    // Obsługa wyłącznika czasowego i manual override
    bool checkTimeout();
    void setPirControlled(bool state);
    bool getPirControlled();
};

#endif