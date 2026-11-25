#ifndef SERVOMANAGER_H
#define SERVOMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define PIN_SERVO_DOOR 27

class ServoManager {
private:
    void setServoAngle(int angle);
    bool isDoorOpen;
    bool isServoActive;
    unsigned long doorOpenTime;
    unsigned long servoMoveStartTime;
    const unsigned long doorOpenDuration = 5000;
    const unsigned long servoDetachDelay = 800; // Czas na wykonanie obrotu (800 ms), potem odcinamy PWM

public:
    ServoManager();
    void init();
    void loop();
    void openDoor();
    void closeDoor();
    bool getIsDoorOpen();
    void processCommand(String jsonCommand);
};

#endif