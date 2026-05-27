#include <Arduino.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "ConnectionManager.h"
#include "LedStripManager.h"
#include "BuzzerManager.h"
#include "FanManager.h"
#include "EnvironmentManager.h"
#include "GasSensorManager.h"
#include "LightSensorManager.h"
#include "WaterSensorManager.h"
#include "RfidManager.h"
#include "ServoManager.h"
#include "DisplayManager.h"
#include "MotionSensorManager.h"
#include "DoorbellManager.h"

// --- ПЕРЕМЕННЫЕ СОСТОЯНИЯ ---
const String AUTHORIZED_CARD_1 = "62CB0951";
const String AUTHORIZED_CARD_2 = "C249BC54";

bool isSystemArmed = false;
bool isNightMode = false;
bool intrusionAlarmActive = false;
bool wrongCardAlarm = false;

// Статусы датчиков
bool waterAlarmActive = false;
bool waterSilenced = false;
bool gasAlarmActive = false;
bool gasSilenced = false;

// Таймеры для фильтрации шума
unsigned long waterTriggerTime = 0;
unsigned long gasTriggerTime = 0;
const unsigned long alarmDelayThreshold = 500;

// RFID таймеры
unsigned long cardStartTime = 0;
bool isCardHeld = false;
const unsigned long armDelay = 5000;
unsigned long lastCardSeenTime = 0;
bool actionExecuted = false;

// Менеджеры
ConnectionManager connectionManager;
LedStripManager ledManager;
BuzzerManager buzzerManager;
FanManager fanManager;
EnvironmentManager envManager;
GasSensorManager gasManager;
LightSensorManager lightManager;
WaterSensorManager waterManager;
RfidManager rfidManager;
ServoManager servoManager;
DisplayManager displayManager;
MotionSensorManager motionManager;
DoorbellManager doorbellManager;

unsigned long lastDisplayUpdate = 0;
const unsigned long displayUpdateInterval = 2000;

// --- ФУНКЦИЯ ПРИНУДИТЕЛЬНОГО ОБНОВЛЕНИЯ СВЕТА ---
void refreshLights() {
  ledManager.processCommand("{\"target\": \"wew\", \"state\": \"" + ledManager.getWewState() + "\"}");
  ledManager.processCommand("{\"target\": \"zew\", \"state\": \"" + ledManager.getZewState() + "\"}");
}

// --- СБРОС ВСЕХ АЛАРМОВ ---
void clearAllAlarms() {
  isSystemArmed = false;
  intrusionAlarmActive = false;
  wrongCardAlarm = false;
  waterAlarmActive = false;
  waterSilenced = false;
  gasAlarmActive = false;
  gasSilenced = false;

  buzzerManager.processCommand("{\"state\": \"OFF\"}");
  ledManager.setAlarmMode(NONE);
  refreshLights();
  displayManager.clearAlarm();

  connectionManager.publishMessage("makieta/access/status", "DISARMED");
  connectionManager.publishMessage("makieta/sensors", "WATER_OK");
  connectionManager.publishMessage("makieta/sensors", "GAS_OK");
}

// --- CALLBACK (ОБРАБОТКА КОМАНД ИЗ ПРИЛОЖЕНИЯ) ---
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) message += (char)payload[i];
  String topicStr = String(topic);

  // 1. Датчики (кнопки STOP)
  if (topicStr == "makieta/sensors/ustaw") {
    if (message == "water_off") {
      waterAlarmActive = false;
      waterSilenced = true;
      buzzerManager.processCommand("{\"state\": \"OFF\"}");
      ledManager.setAlarmMode(NONE);
      refreshLights();
      connectionManager.publishMessage("makieta/sensors", "WATER_OK");
    } else if (message == "gas_off") {
      gasAlarmActive = false;
      gasSilenced = true;
      buzzerManager.processCommand("{\"state\": \"OFF\"}");
      ledManager.setAlarmMode(NONE);
      refreshLights();
      connectionManager.publishMessage("makieta/sensors", "GAS_OK");
    }
  }
  // 2. Доступ / Охрана
  else if (topicStr == "makieta/access/ustaw") {
    if (message == "unlock") {
      servoManager.openDoor();
      displayManager.showRfidMessage("Dostep", 2000);
    } else if (message == "ARM") {
      isSystemArmed = true;
      buzzerManager.triggerBeep(500);
      connectionManager.publishMessage("makieta/access/status", "ARMED");
    } else if (message == "DISARM") {
      clearAllAlarms();
    }
  } else if (topicStr == "makieta/oswietlenie/ustaw") ledManager.processCommand(message);
  else if (topicStr == "makieta/wentylator/ustaw") fanManager.processCommand(message);
  else if (topicStr == "makieta/buzzer/ustaw") buzzerManager.processCommand(message);
  else if (topicStr == "makieta/serwo/ustaw") servoManager.processCommand(message);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  ledManager.init();
  buzzerManager.init();
  fanManager.init();
  servoManager.init();
  envManager.init();
  gasManager.init();
  lightManager.init();
  rfidManager.init();
  motionManager.init();
  doorbellManager.init();
  displayManager.init(WiFi.localIP().toString());

  connectionManager.initWiFi();
  connectionManager.setupMQTT(MQTT_SERVER, MQTT_PORT);
  connectionManager.setCallback(mqttCallback);

  // Убрали проблемные строки connectionManager.subscribe —
  // подписка теперь происходит внутри ConnectionManager.cpp автоматически!

  Serial.println("System Ready");
}

void loop() {
  connectionManager.loop();
  buzzerManager.loop();
  servoManager.loop();
  displayManager.loop();
  envManager.loop();
  ledManager.update();
  motionManager.loop();

  waterManager.loop();
  gasManager.loop();

  lightManager.loop();

  // === ALGORYTM DZIEŃ / NOC I AUTOMATYKA OŚWIETLENIA ZEWNĘTRZNEGO ===
  static unsigned long lastLightControlTime = 0;

  if (millis() - lastLightControlTime > 1000) {
    lastLightControlTime = millis();
    float currentLux = lightManager.getLux();

    const float LUX_THRESHOLD_NIGHT = 100.0;
    const float LUX_THRESHOLD_DAY = 120.0;

    if (currentLux < LUX_THRESHOLD_NIGHT) {
      if (!isNightMode) {
        isNightMode = true;
        connectionManager.publishMessage("makieta/system/tryb", "NIGHT");
      }

      // Dynamiczna regulacja jasności (0 lx = 255, 100 lx = 50)
      int autoBrightness = map((long)currentLux, LUX_THRESHOLD_NIGHT, 0, 50, 255);
      autoBrightness = constrain(autoBrightness, 50, 255);

      // Generowanie i wysyłanie komendy do paska LED
      String jsonCommand = "{\"target\": \"zew\", \"state\": \"ON\", \"brightness\": " + String(autoBrightness) + ", \"color\": {\"r\": 255, \"g\": 255, \"b\": 255}}";
      ledManager.processCommand(jsonCommand);

    } else if (currentLux > LUX_THRESHOLD_DAY) {
      if (isNightMode) {
        isNightMode = false;
        connectionManager.publishMessage("makieta/system/tryb", "DAY");

        String jsonCommand = "{\"target\": \"zew\", \"state\": \"OFF\"}";
        ledManager.processCommand(jsonCommand);
      }
    }

    // === AKTUALIZACJA WYŚWIETLACZA OLED ===
    // Pobieramy faktyczne stany z LedStripManager i przekazujemy na ekran
    displayManager.updateLighting(
      (int)currentLux,
      ledManager.getWewState(),
      ledManager.getZewState());
  }

  // --- ЛОГИКА ВОДЫ ---
  if (waterManager.isAlarmActive()) {
    if (waterTriggerTime == 0) waterTriggerTime = millis();
    if (!waterAlarmActive && !waterSilenced && (millis() - waterTriggerTime > alarmDelayThreshold)) {
      waterAlarmActive = true;
      connectionManager.publishMessage("makieta/sensors", "WATER_ALARM");
      buzzerManager.processCommand("{\"state\": \"ON\"}");
      ledManager.setAlarmMode(WATER_ALARM);

      displayManager.triggerAlarm("WYKRYTO ZALANIE");
    }
  } else {
    waterTriggerTime = 0;
    if (waterAlarmActive || waterSilenced) {
      waterAlarmActive = false;
      waterSilenced = false;
      connectionManager.publishMessage("makieta/sensors", "WATER_OK");
      if (!gasAlarmActive && !intrusionAlarmActive) {
        buzzerManager.processCommand("{\"state\": \"OFF\"}");
        ledManager.setAlarmMode(NONE);
        refreshLights();
        displayManager.clearAlarm();
      }
    }
  }

  // --- ЛОГИКА ГАЗА ---
  if (gasManager.isAlarmActive()) {
    if (gasTriggerTime == 0) gasTriggerTime = millis();
    if (!gasAlarmActive && !gasSilenced && (millis() - gasTriggerTime > alarmDelayThreshold)) {
      gasAlarmActive = true;
      connectionManager.publishMessage("makieta/sensors", "GAS_ALARM");
      buzzerManager.processCommand("{\"state\": \"ON\"}");
      ledManager.setAlarmMode(GAS_ALARM);

      displayManager.triggerAlarm("WYKRYTO GAZ/DYM");
    }
  } else {
    gasTriggerTime = 0;
    if (gasAlarmActive || gasSilenced) {
      gasAlarmActive = false;
      gasSilenced = false;
      connectionManager.publishMessage("makieta/sensors", "GAS_OK");
      if (!waterAlarmActive && !intrusionAlarmActive) {
        buzzerManager.processCommand("{\"state\": \"OFF\"}");
        ledManager.setAlarmMode(NONE);
        refreshLights();
        displayManager.clearAlarm();
      }
    }
  }

  // Heartbeat для приложения (раз в 5 сек)
  static unsigned long lastAppUpdate = 0;
  if (millis() - lastAppUpdate > 5000) {
    lastAppUpdate = millis();
    connectionManager.publishMessage("makieta/sensors", waterAlarmActive ? "WATER_ALARM" : "WATER_OK");
    connectionManager.publishMessage("makieta/sensors", gasAlarmActive ? "GAS_ALARM" : "GAS_OK");
    connectionManager.publishMessage("makieta/sensors", "HUM:" + String((int)envManager.getHum()));
    connectionManager.publishMessage("makieta/czujniki/swiatlo", lightManager.getSensorJson());
  }

// === AKTUALIZACJA WYŚWIETLACZA OLED ===
  if (millis() - lastDisplayUpdate >= displayUpdateInterval) {
    lastDisplayUpdate = millis();
    
    // Przekazanie flagi - DisplayManager sam spokojnie włączy ekran "SYSTEM UZBROJONY"
    displayManager.updateSystemState(isSystemArmed);
    
    // Pobieranie danych w tle
    displayManager.updateEnvironment(envManager.getTemp(), envManager.getHum(), fanManager.getSpeed());
  }

  if (motionManager.getIsEnabled() && motionManager.isMotion()) {
    if (isSystemArmed && !intrusionAlarmActive) {
      intrusionAlarmActive = true;
      connectionManager.publishMessage("makieta/access/status", "ALARM");
      buzzerManager.processCommand("{\"state\": \"ON\"}");
      ledManager.setAlarmMode(INTRUSION_ALARM);
      displayManager.triggerAlarm("WYKRYTO RUCH!");
    }
  }

  if (rfidManager.loop()) {
    String scannedUID = rfidManager.getUID();
    lastCardSeenTime = millis();
    if (!isCardHeld) {
      cardStartTime = millis();
      isCardHeld = true;
      actionExecuted = false;
      connectionManager.publishMessage("makieta/access/status", scannedUID);
    }
    if (scannedUID == AUTHORIZED_CARD_1) {
      if (intrusionAlarmActive || wrongCardAlarm) clearAllAlarms();
      if (!actionExecuted && millis() - cardStartTime >= armDelay) {
        isSystemArmed = !isSystemArmed;
        actionExecuted = true;
        connectionManager.publishMessage("makieta/access/status", isSystemArmed ? "ARMED" : "DISARMED");
        if (isSystemArmed) buzzerManager.triggerBeep(500);
        else clearAllAlarms();
      }
    } else {
      if (!wrongCardAlarm) {
        wrongCardAlarm = true;
        connectionManager.publishMessage("makieta/access/status", "DENIED");
        ledManager.setAlarmMode(INTRUSION_ALARM);
        buzzerManager.processCommand("{\"state\": \"ON\"}");
      }
    }
  } else {
    if (isCardHeld && (millis() - lastCardSeenTime > 500)) {
      if ((lastCardSeenTime - cardStartTime) < armDelay && !actionExecuted && !isSystemArmed) servoManager.openDoor();
      isCardHeld = false;
    }
  }
  if (doorbellManager.isRinging()) buzzerManager.triggerDoorbell();
}