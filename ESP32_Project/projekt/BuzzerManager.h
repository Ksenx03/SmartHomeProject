#ifndef BUZZERMANAGER_H
#define BUZZERMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define PIN_BUZZER 25

class BuzzerManager {
private:
  bool isBeeping;  // Flaga informująca, czy aktualnie trwa "piknięcie"
  bool isDingDong;
  unsigned long beepEndTime;   // Znacznik czasu, kiedy buzzer ma zamilknąć
  unsigned long nextToneTime;  // Czas przejścia z "Ding" na "Dong"

  void setBuzzerState(bool state);  // Fizyczne ustawienie pinu
  void playTone(int frequency, int duration); // Nowa metoda pomocnicza

public:
  BuzzerManager();
  void init();
  void processCommand(String jsonCommand);
  void loop();  // Asynchroniczna pętla dla buzzera

  // Publiczna metoda, którą w przyszłości wywołamy np. z klasy od RFID
  void triggerBeep(unsigned int durationMs);
  void triggerDoorbell(); // Nowa dedykowana metoda dla dzwonka
};

#endif