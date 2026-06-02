#ifndef WATERSENSORMANAGER_H
#define WATERSENSORMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define PIN_WATER 35
#define WATER_THRESHOLD 1000

class WaterSensorManager {
private:
  unsigned long lastReadTime;
  const unsigned long readInterval = 2000; 

  int rawValue;
  int waterLevelPercentage;
  bool isAlarm;

public:
  WaterSensorManager();
  void init();
  bool loop();
  String getSensorJson();
  bool getAlarmState();
  bool isAlarmActive();
};

#endif