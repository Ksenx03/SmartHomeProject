#ifndef GASSENSORMANAGER_H
#define GASSENSORMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define PIN_MQ2 34

// Próg alarmowy - do kalibracji podczas testów (wartość z ADC od 0 do 4095)
#define GAS_THRESHOLD 2000

class GasSensorManager {
private:
  unsigned long lastReadTime;
  const unsigned long readInterval = 2000;  // Interwał: co 2 sekundy

  int rawValue;
  int gasPercentage;
  bool isAlarm;

public:
  GasSensorManager();
  void init();

  // Zwraca true, jeśli są nowe dane
  bool loop();

  // Generuje paczkę JSON
  String getSensorJson();

  // Dodatkowy getter, jeśli w przyszłości będziemy chcieli odpalić syrenę z innej klasy
  bool getAlarmState();
  bool isAlarmActive();
};

#endif