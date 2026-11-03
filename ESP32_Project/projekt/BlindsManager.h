#ifndef BLINDSMANAGER_H
#define BLINDSMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#define PIN_SERVO_BLIND 14

typedef void (*BlindsCallback)(int position);

class BlindsManager {
private:
    void setBlindPWM(int duty);
    void savePosition();

    bool isBlindMoving;
    unsigned long blindMoveStartTime;
    unsigned long requiredMoveDuration;
    
    const unsigned long totalTravelTime = 7000;
    
    int currentPosition;
    int targetPosition;

    Preferences prefs;
    BlindsCallback onPositionChanged = nullptr;

public:
    BlindsManager();
    void init();
    void loop();
    
    void setPosition(int targetPercent);
    void openBlinds();
    void closeBlinds();
    void stopBlinds();

    void setPositionCallback(BlindsCallback cb) { onPositionChanged = cb; }
    
    int getCurrentPosition() const { return currentPosition; }
    void processCommand(String jsonCommand);
};

#endif