#include "DisplayManager.h"

// Konstruktor - inicjalizacja obiektu SSD1306
DisplayManager::DisplayManager()
  : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {
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
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Nie znaleziono ekranu OLED!"));
    return false;
  }

  display.setRotation(2);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // Ekran startowy
  display.setCursor(10, 25);
  display.println("Smart Home IoT");
  display.setCursor(10, 40);
  display.println("Inicjalizacja...");
  display.display();
  delay(1000);  // Jedyny delay, dopuszczalny tylko podczas inicjalizacji (setup)

  return true;
}

void DisplayManager::loop() {
  display.clearDisplay();

  // 1. Obsługa ekranu tymczasowego RFID
  if (currentState == SCREEN_RFID) {
    if (millis() - temporaryScreenTimer > temporaryScreenDuration) {
      currentState = previousState;
    }
  }

  // 2. Logika karuzeli i wymuszania ekranu uzbrojenia
  if (currentState != SCREEN_ALARM && currentState != SCREEN_RFID) {
    if (isArmed) {
      currentState = SCREEN_ARMED;
    } else {
      if (currentState == SCREEN_ARMED) {
        currentState = SCREEN_ENV;
        lastScreenChange = millis();
      }

      if (millis() - lastScreenChange > screenInterval) {
        lastScreenChange = millis();
        if (currentState == SCREEN_ENV) {
          currentState = SCREEN_LIGHTS;
        } else if (currentState == SCREEN_LIGHTS) {
          currentState = SCREEN_ENV;
        }
      }
    }
  }

  // === ZABEZPIECZENIE: BLOKADA MIGANIA ===
  // Jeśli aktualny stan to NIE JEST krytyczny alarm, wymuszamy brak inwersji
  if (currentState != SCREEN_ALARM) {
    display.invertDisplay(false);
  }
  // =======================================

  // 3. Renderowanie odpowiedniego ekranu na podstawie stanu
  switch (currentState) {
    case SCREEN_ENV: drawEnvScreen(); break;
    case SCREEN_LIGHTS: drawLightsScreen(); break;
    case SCREEN_ALARM: drawAlarmScreen(); break;
    case SCREEN_RFID: drawRfidScreen(); break;
    case SCREEN_ARMED: drawArmedScreen(); break;
  }

  display.display();
}

// ==========================================
// EKRAN: SYSTEM UZBROJONY (Spokojny, statyczny)
// ==========================================
void DisplayManager::drawArmedScreen() {
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  // Duży, czytelny napis na środku ekranu - bez żadnego migania
  display.setCursor(25, 15);
  display.print("SYSTEM");

  display.setCursor(10, 40);
  display.print("UZBROJONY");
}

// ==========================================
// ZMODYFIKOWANY EKRAN ALARMU (Miga + Polski tekst)
// ==========================================
void DisplayManager::drawAlarmScreen() {
  // Dynamiczne miganie - inwersja ekranu co 500ms
  display.invertDisplay(millis() % 1000 < 500);

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  // Napis ! ALARM !
  display.setCursor(12, 10);
  display.print("! ALARM !");

  // Przyczyna alarmu
  display.setTextSize(1);
  display.setCursor(10, 40);
  display.print(alarmReason);
}

void DisplayManager::drawTopBar() {
  // Funkcja pozostawiona pusta celowo
}

// 2. Nowa konfiguracja ekranu środowiskowego (SCREEN_ENV)
void DisplayManager::drawEnvScreen() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // --- LINIA 1: Kompaktowy odczyt głównych parametrów ---
  // Przesunięcie na Y=12 zapewnia ładne symetryczne pozycjonowanie na ekranie
  display.setCursor(10, 12);
  display.print(temperature, 1);
  display.print((char)247);  // Wyświetla profesjonalny symbol stopnia: °
  display.print("C  |  ");
  display.print(humidity, 1);
  display.print(" %");

  // --- Estetyczny separator graficzny ---
  // Rysuje poziomą linię od piksela 0 do 128 na wysokości Y=26
  display.drawFastHLine(0, 26, 128, SSD1306_WHITE);

  // --- LINIA 2: Stan wentylatora ---
  display.setCursor(10, 36);
  display.print("Wentylator : ");
  if (fanSpeed == 0) {
    display.println("OFF");
  } else {
    display.print(fanSpeed);
    display.println("%");
  }

  // --- LINIA 3: Stan ogrzewania (wpisany na sztywno) ---
  display.setCursor(10, 48);
  display.println("Ogrzewanie : OFF");
}

void DisplayManager::drawLightsScreen() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // --- LINIA 1: Jasność otoczenia i tryb pracy oświetlenia ---
  display.setCursor(10, 12);
  display.print(luxLevel);
  display.print(" lx  |  ");

  if (luxLevel < 200) {
    display.print("NIGHT");
  } else {
    display.print("DAY");
  }

  // --- Estetyczny separator graficzny ---
  display.drawFastHLine(0, 26, 128, SSD1306_WHITE);

  // --- LINIA 2: Pasek LED wewnętrzny (LED IN) ---
  display.setCursor(10, 36);
  display.print("LED IN: ");
  display.print(ledStateWew);

  if (ledStateWew == "ON") {
    display.print(" | 75%");
  }

  // --- LINIA 3: Pasek LED zewnętrzny (LED OUT) ---
  display.setCursor(10, 48);
  display.print("LED OUT: ");
  display.print(ledStateZew);

  if (ledStateZew == "ON") {
    display.print(" | 30%");
  }
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
  currentState = SCREEN_ALARM;  // Wymuszamy stan priorytetowy
}

void DisplayManager::clearAlarm() {
  // Wyłączamy inwersję w razie gdyby została po miganiu
  display.invertDisplay(false);
  currentState = SCREEN_ENV;
}

void DisplayManager::showRfidMessage(String msg, unsigned long durationMs) {
  if (currentState == SCREEN_ALARM) return;  // Alarmu nie przerywamy komunikatem RFID

  rfidMessage = msg;
  previousState = currentState;  // Zapisujemy, czy byliśmy w ENV czy w LIGHTS
  currentState = SCREEN_RFID;
  temporaryScreenTimer = millis();
  temporaryScreenDuration = durationMs;
}