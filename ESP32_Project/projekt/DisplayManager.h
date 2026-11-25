#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

enum ScreenState {
    SCREEN_ENV,
    SCREEN_LIGHTS,
    SCREEN_ALARM,
    SCREEN_RFID,
    SCREEN_ARMED
};

class DisplayManager {
private:
    Adafruit_SSD1306 display;
    
    ScreenState currentState;
    ScreenState previousState;
    unsigned long lastScreenChange;
    const unsigned long screenInterval = 4000;
    
    unsigned long temporaryScreenTimer;
    unsigned long temporaryScreenDuration;

    String currentIP;
    bool isConnectedMQTT;
    bool isArmed;
    
    float temperature;
    float humidity;
    int fanSpeed;
    bool heatingActive;
    
    int luxLevel;
    String ledStateWew;
    String ledStateZew;
    
    String alarmReason;
    String rfidMessage;

    void drawTopBar();
    void drawEnvScreen();
    void drawLightsScreen();
    void drawAlarmScreen();
    void drawRfidScreen();
    void drawArmedScreen();

public:
    DisplayManager();
    bool init(String ip);
    void loop();
    
    void updateNetworkStatus(bool mqttStatus, String ip);
    void updateEnvironment(float temp, float hum, int fan, bool heating = false);
    void updateLighting(int lux, String wew, String zew);
    void updateSystemState(bool armed);
    
    void triggerAlarm(String reason);
    void clearAlarm();
    void showRfidMessage(String msg, unsigned long durationMs = 2000);
};

#endif