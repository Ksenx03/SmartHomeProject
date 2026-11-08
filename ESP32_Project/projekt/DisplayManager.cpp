#include "DisplayManager.h"

DisplayManager::DisplayManager()
  : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {
  currentState = SCREEN_ENV;
  previousState = SCREEN_ENV;
  lastScreenChange = 0;
  temporaryScreenTimer = 0;

  currentIP = "Brak IP";
  isConnectedMQTT = false;
  isArmed = false;
  temperature = 0.0;
  humidity = 0.0;
  fanSpeed = 0;
  heatingActive = false;
  luxLevel = 0;
  ledStateWew = "OFF";
  ledStateZew = "OFF";
}

bool DisplayManager::init(String ip) {
  currentIP = ip;

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Nie znaleziono ekranu OLED!"));
    return false;
  }

  display.setRotation(2);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(10, 25);
  display.println("Smart Home IoT");
  display.setCursor(10, 40);
  display.println("Inicjalizacja...");
  display.display();
  delay(1000);

  return true;
}

void DisplayManager::loop() {
  static unsigned long lastOledRender = 0;
  if (millis() - lastOledRender < 250) {
    return; // Odświeżaj ekran maksymalnie co 250 ms - nie blokuj I2C i SPI!
  }
  lastOledRender = millis();

  display.clearDisplay();

  if (currentState == SCREEN_RFID) {
    if (millis() - temporaryScreenTimer > temporaryScreenDuration) {
      currentState = previousState;
    }
  }

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

  if (currentState != SCREEN_ALARM) {
    display.invertDisplay(false);
  }

  switch (currentState) {
    case SCREEN_ENV: drawEnvScreen(); break;
    case SCREEN_LIGHTS: drawLightsScreen(); break;
    case SCREEN_ALARM: drawAlarmScreen(); break;
    case SCREEN_RFID: drawRfidScreen(); break;
    case SCREEN_ARMED: drawArmedScreen(); break;
  }

  display.display();
}

void DisplayManager::drawArmedScreen() {
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(25, 15);
  display.print("SYSTEM");
  display.setCursor(10, 40);
  display.print("UZBROJONY");
}

void DisplayManager::drawAlarmScreen() {
  display.invertDisplay(millis() % 1000 < 500);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(12, 10);
  display.print("! ALARM !");

  display.setTextSize(1);
  display.setCursor(10, 40);
  display.print(alarmReason);
}

void DisplayManager::drawTopBar() {}

void DisplayManager::drawEnvScreen() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(6, 12);
  display.print(temperature, 2);
  display.print((char)247);
  display.print("C | ");
  display.print(humidity, 2);
  display.print("%");

  display.drawFastHLine(0, 26, 128, SSD1306_WHITE);

  display.setCursor(10, 36);
  display.print("Wentylator : ");
  if (fanSpeed == 0) {
    display.println("OFF");
  } else {
    display.print(fanSpeed);
    display.println("%");
  }

  display.setCursor(10, 48);
  display.print("Ogrzewanie : ");
  display.println(heatingActive ? "ON" : "OFF");
}

void DisplayManager::drawLightsScreen() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(10, 12);
  display.print(luxLevel);
  display.print(" lx  |  ");

  if (luxLevel < 200) {
    display.print("NIGHT");
  } else {
    display.print("DAY");
  }

  display.drawFastHLine(0, 26, 128, SSD1306_WHITE);

  display.setCursor(10, 36);
  display.print("LED IN: ");
  display.print(ledStateWew);

  if (ledStateWew == "ON") {
    display.print(" | 75%");
  }

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

void DisplayManager::updateEnvironment(float temp, float hum, int fan, bool heating) {
  temperature = temp;
  humidity = hum;
  fanSpeed = fan;
  heatingActive = heating;
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
  currentState = SCREEN_ALARM;
}

void DisplayManager::clearAlarm() {
  display.invertDisplay(false);
  currentState = SCREEN_ENV;
}

void DisplayManager::showRfidMessage(String msg, unsigned long durationMs) {
  if (currentState == SCREEN_ALARM) return;

  rfidMessage = msg;
  if (currentState != SCREEN_RFID) {
    previousState = currentState;
  }
  currentState = SCREEN_RFID;
  temporaryScreenTimer = millis();
  temporaryScreenDuration = durationMs;
}