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
    void updatePositionInFlight(); // Przelicza pozycję, jeśli przerwano ruch

    bool isBlindMoving;
    unsigned long blindMoveStartTime;
    unsigned long requiredMoveDuration;
    
    const unsigned long totalTravelTime = 4200; // Czas pełnego przejazdu 0-100% (7s)
    
    int startPosition;   // Pozycja, z której zaczął się obecny ruch
    int currentPosition; // Aktualna pozycja (0 = otwarte, 100 = zamknięte)
    int targetPosition;  // Pozycja docelowa
    
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
    void publishCurrentState(); // Wysyła aktualny stan po połączeniu z MQTT
    void processCommand(String jsonCommand);
};

#endif