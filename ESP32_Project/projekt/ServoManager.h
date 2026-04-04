#ifndef SERVOMANAGER_H
#define SERVOMANAGER_H

#include <Arduino.h>
//#include <ESP32Servo.h>
#include <ArduinoJson.h>

// #define PIN_SERVO_BLIND 14 // Tymczasowo zakomentowane (Żaluzja)
#define PIN_SERVO_DOOR 27

class ServoManager {
private:
    // Servo blindServo; // Tymczasowo zakomentowane
    //Servo doorServo;
    void setServoAngle(int angle);
    bool isDoorOpen;
    unsigned long doorOpenTime;
    const unsigned long doorOpenDuration = 5000; // Drzwi otwarte przez 5 sekund

public:
    ServoManager();
    void init();
    void loop(); // Asynchroniczne sprawdzanie czasu

    void openDoor();   // Obrót o 100 stopni
    void closeDoor();  // Powrót do 0 stopni

    void processCommand(String jsonCommand);
    
    // void setBlindPosition(int angle); // Tymczasowo zakomentowane
};

#endif