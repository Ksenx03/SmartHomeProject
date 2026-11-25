#ifndef BUZZERMANAGER_H
#define BUZZERMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define PIN_BUZZER 25

class BuzzerManager {
private:
  bool isBeeping;
  bool isDingDong;
  unsigned long beepEndTime;
  unsigned long nextToneTime;

  void setBuzzerState(bool state);

public:
  BuzzerManager();
  void init();
  void processCommand(String jsonCommand);
  void loop();

  void triggerBeep(unsigned int durationMs);
  void triggerDoorbell();
};

#endif