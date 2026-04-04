#ifndef WATERSENSORMANAGER_H
#define WATERSENSORMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define PIN_WATER 35

// Próg alarmowy - do kalibracji (od 0 do 4095).
// Powyżej tej wartości aplikacja uzna, że doszło do zalania.
#define WATER_THRESHOLD 1000

class WaterSensorManager {
private:
  unsigned long lastReadTime;
  const unsigned long readInterval = 2000;  // Interwał: co 2 sekundy

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