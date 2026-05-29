#ifndef BLINDSMANAGER_H
#define BLINDSMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define PIN_SERVO_BLIND 14 // Dedukowany pin GPIO 14 dla serwa pracy ciągłej (360)

class BlindsManager {
private:
    void setBlindPWM(int duty);
    
    bool isBlindMoving;
    unsigned long blindMoveTime;
    const unsigned long blindMoveDuration = 3000; // Czas pełnego cyklu ruchu rolety (np. 3 sekundy)

public:
    BlindsManager();
    void init();
    void loop(); // Odpowiada za asynchroniczne wyłączenie silnika po zadanym czasie
    
    void openBlinds();
    void closeBlinds();
    void stopBlinds();
    
    void processCommand(String jsonCommand);
};

#endif