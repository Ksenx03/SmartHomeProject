#include "DisplayManager.h"

// Konstruktor - inicjalizacja obiektu SSD1306
DisplayManager::DisplayManager() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {
    currentState = SCREEN_ENV;
    previousState = SCREEN_ENV;
    lastScreenChange = 0;
    temporaryScreenTimer = 0;
    
    // Wartości domyślne
    currentIP = "Brak IP";
    isConnectedMQTT = false;
    isArmed = false;
    temperature = 0.0;
    humidity = 0.0;
    fanSpeed = 0;
    luxLevel = 0;
    ledStateWew = "OFF";
    ledStateZew = "OFF";
}

bool DisplayManager::init(String ip) {
    currentIP = ip;
    
    // Inicjalizacja ekranu na domyślnych pinach I2C ESP32 (SDA=21, SCL=22)
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("Nie znaleziono ekranu OLED!"));
        return false;
    }
    
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    
    // Ekran startowy
    display.setCursor(10, 25);
    display.println("Smart Home IoT");
    display.setCursor(10, 40);
    display.println("Inicjalizacja...");
    display.display();
    delay(1000); // Jedyny delay, dopuszczalny tylko podczas inicjalizacji (setup)
    
    return true;
}

void DisplayManager::loop() {
    unsigned long currentMillis = millis();

    // 1. Obsługa ekranów tymczasowych (RFID)
    if (currentState == SCREEN_RFID) {
        if (currentMillis - temporaryScreenTimer >= temporaryScreenDuration) {
            // Koniec czasu wyświetlania komunikatu, wracamy do poprzedniego ekranu
            currentState = previousState; 
        }
    } 
    // 2. Cykliczna zmiana ekranów standardowych (jeśli nie ma alarmu)
    else if (currentState != SCREEN_ALARM) {
        if (currentMillis - lastScreenChange >= screenInterval) {
            lastScreenChange = currentMillis;
            
            // Przełączanie karuzeli
            if (currentState == SCREEN_ENV) {
                currentState = SCREEN_LIGHTS;
            } else {
                currentState = SCREEN_ENV;
            }
        }
    }

    // 3. Rysowanie odpowiedniego ekranu
    display.clearDisplay();
    
    // Pasek górny rysujemy zawsze, chyba że jest to alarm pełnoekranowy
    if (currentState != SCREEN_ALARM) {
        drawTopBar();
    }

    switch (currentState) {
        case SCREEN_ENV:
            drawEnvScreen();
            break;
        case SCREEN_LIGHTS:
            drawLightsScreen();
            break;
        case SCREEN_ALARM:
            drawAlarmScreen();
            break;
        case SCREEN_RFID:
            drawRfidScreen();
            break;
    }
    
    display.display(); // Wysłanie bufora do ekranu
}

void DisplayManager::drawTopBar() {
    display.setTextSize(1);
    display.setCursor(0, 0);
    
    // Status MQTT
    if (isConnectedMQTT) display.print("[M] ");
    else display.print("[x] ");
    
    // Status Alarmu
    if (isArmed) display.print("ALARM:ON ");
    else display.print("ALARM:OFF ");
    
    // Linia oddzielająca
    display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
}

void DisplayManager::drawEnvScreen() {
    display.setTextSize(1);
    
    display.setCursor(0, 16);
    display.print("Srodowisko & Klima");
    
    display.setCursor(0, 30);
    display.print("Temp: "); display.print(temperature, 1); display.print(" C");
    
    display.setCursor(0, 42);
    display.print("Wilg: "); display.print(humidity, 1); display.print(" %");
    
    display.setCursor(0, 54);
    display.print("Went: "); 
    if (fanSpeed == 0) display.print("OFF");
    else { display.print(fanSpeed); display.print(" %"); }
}

void DisplayManager::drawLightsScreen() {
    display.setTextSize(1);
    
    display.setCursor(0, 16);
    display.print("Oswietlenie & Zewn.");
    
    display.setCursor(0, 30);
    display.print("Jasnosc: "); display.print(luxLevel); display.print(" lx");
    
    display.setCursor(0, 42);
    display.print("LED Wewn: "); display.print(ledStateWew);
    
    display.setCursor(0, 54);
    display.print("LED Zewn: "); display.print(ledStateZew);
}

void DisplayManager::drawAlarmScreen() {
    // Odwrócenie kolorów dla lepszego efektu
    display.invertDisplay(millis() % 1000 < 500); // Miganie co pół sekundy
    
    display.setTextSize(2);
    display.setCursor(15, 10);
    display.print("! ALARM !");
    
    display.setTextSize(1);
    display.setCursor(15, 35);
    display.print("Wykryto zagrozenie:");
    
    display.setCursor(15, 50);
    display.setTextSize(1);
    display.print(alarmReason);
}

void DisplayManager::drawRfidScreen() {
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.print("Zblizono karte:");
    
    display.setCursor(0, 40);
    display.setTextSize(1);
    display.print(rfidMessage);
}

// --- METODY AKTUALIZUJĄCE DANE (Settery) ---

void DisplayManager::updateEnvironment(float temp, float hum, int fan) {
    temperature = temp;
    humidity = hum;
    fanSpeed = fan;
}

void DisplayManager::updateLighting(int lux, String wew, String zew) {
    luxLevel = lux;
    ledStateWew = wew;
    ledStateZew = zew;
}

void DisplayManager::updateSystemState(bool armed) {
    isArmed = armed;
}

void DisplayManager::updateNetworkStatus(bool mqttStatus, String ip) {
    isConnectedMQTT = mqttStatus;
    currentIP = ip;
}

void DisplayManager::triggerAlarm(String reason) {
    alarmReason = reason;
    currentState = SCREEN_ALARM; // Wymuszamy stan priorytetowy
}

void DisplayManager::clearAlarm() {
    // Wyłączamy inwersję w razie gdyby została po miganiu
    display.invertDisplay(false); 
    currentState = SCREEN_ENV;
}

void DisplayManager::showRfidMessage(String msg, unsigned long durationMs) {
    if (currentState == SCREEN_ALARM) return; // Alarmu nie przerywamy komunikatem RFID
    
    rfidMessage = msg;
    previousState = currentState; // Zapisujemy, czy byliśmy w ENV czy w LIGHTS
    currentState = SCREEN_RFID;
    temporaryScreenTimer = millis();
    temporaryScreenDuration = durationMs;
}