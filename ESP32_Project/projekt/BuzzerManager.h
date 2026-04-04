#ifndef BUZZERMANAGER_H
#define BUZZERMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define PIN_BUZZER 25

class BuzzerManager {
private:
    bool isBeeping;            // Flaga informująca, czy aktualnie trwa "piknięcie"
    unsigned long beepEndTime; // Znacznik czasu, kiedy buzzer ma zamilknąć

    void setBuzzerState(bool state); // Fizyczne ustawienie pinu

public:
    BuzzerManager();
    void init();
    void processCommand(String jsonCommand);
    void loop(); // Asynchroniczna pętla dla buzzera
    
    // Publiczna metoda, którą w przyszłości wywołamy np. z klasy od RFID
    void triggerBeep(unsigned int durationMs); 
};

#endif