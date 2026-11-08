#ifndef GASSENSORMANAGER_H
#define GASSENSORMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define PIN_MQ2 34
#define GAS_THRESHOLD 2000

class GasSensorManager {
private:
  unsigned long lastReadTime;
  const unsigned long readInterval = 2000;

  int rawValue;
  int gasPercentage;
  bool isAlarm;

public:
  GasSensorManager();
  void init();
  bool loop();
  String getSensorJson();
  bool getAlarmState();
  bool isAlarmActive();
  int getRawValue();
  int getPercentage();
};

#endif