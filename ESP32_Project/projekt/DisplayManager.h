#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Definicje parametrów wyświetlacza
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 // Piny resetu (-1 jeśli współdzielony z pinem resetu mikrokontrolera)
#define SCREEN_ADDRESS 0x3C // Standardowy adres I2C dla wyświetlaczy 128x64

// Maszyna stanów dla ekranów
enum ScreenState {
    SCREEN_ENV,     // Ekran 1: Temperatura, wilgotność, wentylator
    SCREEN_LIGHTS,  // Ekran 2: Oświetlenie, jasność
    SCREEN_ALARM,   // Ekran priorytetowy: Alarm krytyczny
    SCREEN_RFID     // Ekran priorytetowy: Komunikat z czytnika kart
};

class DisplayManager {
private:
    Adafruit_SSD1306 display;
    
    // Zmienne obsługujące maszynę stanów i czas
    ScreenState currentState;
    ScreenState previousState; // Do powrotu po ekranie tymczasowym (np. RFID)
    unsigned long lastScreenChange;
    const unsigned long screenInterval = 4000; // Czas wyświetlania jednego ekranu (4 sekundy)
    
    unsigned long temporaryScreenTimer;
    unsigned long temporaryScreenDuration;

    // Bufory danych do wyświetlenia
    String currentIP;
    bool isConnectedMQTT;
    bool isArmed;
    
    float temperature;
    float humidity;
    int fanSpeed;
    
    int luxLevel;
    String ledStateWew;
    String ledStateZew;
    
    String alarmReason;
    String rfidMessage;

    // Prywatne metody rysujące poszczególne elementy
    void drawTopBar();
    void drawEnvScreen();
    void drawLightsScreen();
    void drawAlarmScreen();
    void drawRfidScreen();

public:
    DisplayManager();
    
    // Inicjalizacja magistrali I2C i ekranu
    bool init(String ip);
    
    // Główna funkcja wywoływana w loop()
    void loop();
    
    // Metody aktualizujące dane (wywoływane po odebraniu danych z czujników/MQTT)
    void updateNetworkStatus(bool mqttStatus, String ip);
    void updateEnvironment(float temp, float hum, int fan);
    void updateLighting(int lux, String wew, String zew);
    void updateSystemState(bool armed);
    
    // Metody wyzwalające zdarzenia priorytetowe
    void triggerAlarm(String reason);
    void clearAlarm();
    void showRfidMessage(String msg, unsigned long durationMs = 2000);
};

#endif