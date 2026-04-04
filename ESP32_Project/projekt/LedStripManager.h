#ifndef LEDSTRIPMANAGER_H
#define LEDSTRIPMANAGER_H

#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

#define PIN_LED_WEW 12
#define PIN_LED_ZEW 13
#define NUM_LEDS_PER_STRIP 30

enum AlarmMode { NONE,
                 GAS_ALARM,
                 WATER_ALARM,
                 INTRUSION_ALARM };

class LedStripManager {
private:
  Adafruit_NeoPixel stripWew;
  Adafruit_NeoPixel stripZew;

  bool wewIsOn;
  bool zewIsOn;

  AlarmMode currentAlarmMode = NONE;

public:
  LedStripManager();
  void init();
  void processCommand(String jsonCommand);

  String getWewState();
  String getZewState();

  void setAlarmMode(AlarmMode mode);
  void update();  // Metoda wywoływana w pętli loop()
};

#endif