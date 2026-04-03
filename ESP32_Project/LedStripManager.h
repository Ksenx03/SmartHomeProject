#ifndef LEDSTRIPMANAGER_H
#define LEDSTRIPMANAGER_H

#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

#define PIN_LED_WEW 12
#define PIN_LED_ZEW 13
#define NUM_LEDS_PER_STRIP 30

class LedStripManager {
private:
    // Tworzymy dwa całkowicie niezależne obiekty dla każdego paska
    Adafruit_NeoPixel stripWew;
    Adafruit_NeoPixel stripZew;

public:
    LedStripManager();
    void init();
    void processCommand(String jsonCommand);
};

#endif