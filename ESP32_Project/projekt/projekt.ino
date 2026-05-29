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
#include "IndoorEnvironmentManager.h"
#include "BlindsManager.h"

const String AUTHORIZED_CARD_1 = "62CB0951";
const String AUTHORIZED_CARD_2 = "C249BC54";

bool isSystemArmed = false;
bool isNightMode = false;
bool intrusionAlarmActive = false;
bool wrongCardAlarm = false;

bool waterAlarmActive = false;
bool waterSilenced = false;
bool gasAlarmActive = false;
bool gasSilenced = false;

bool isAutoLightModeEnabled = true;

unsigned long waterTriggerTime = 0;
unsigned long gasTriggerTime = 0;
const unsigned long alarmDelayThreshold = 500;

unsigned long cardStartTime = 0;
bool isCardHeld = false;
const unsigned long armDelay = 5000;
unsigned long lastCardSeenTime = 0;
bool actionExecuted = false;

ConnectionManager connectionManager;
LedStripManager ledManager;
BuzzerManager buzzerManager;
FanManager fanManager;
GasSensorManager gasManager;
LightSensorManager lightManager;
WaterSensorManager waterManager;
RfidManager rfidManager;
ServoManager servoManager;
DisplayManager displayManager;
MotionSensorManager motionManager;
DoorbellManager doorbellManager;
EnvironmentManager envManager;
IndoorEnvironmentManager indoorEnvManager;
BlindsManager blindsManager;

unsigned long lastDisplayUpdate = 0;
const unsigned long displayUpdateInterval = 2000;

void refreshLights() {
  ledManager.processCommand("{\"target\": \"wew\", \"state\": \"" + ledManager.getWewState() + "\"}");
  ledManager.processCommand("{\"target\": \"zew\", \"state\": \"" + ledManager.getZewState() + "\"}");
}

// Pomocnicza funkcja generująca podwójne piknięcie "pik-pik" (Uzbrojenie)
void playArmSound() {
  // Pierwsze krótkie "pik"
  buzzerManager.processCommand("{\"state\": \"ON\"}");
  delay(120);
  buzzerManager.processCommand("{\"state\": \"OFF\"}");
  delay(80);  // przerwa

  // Drugie krótkie "pik"
  buzzerManager.processCommand("{\"state\": \"ON\"}");
  delay(120);
  buzzerManager.processCommand("{\"state\": \"OFF\"}");
  delay(80);  // przerwa przed długim sygnałem

  // Trzecie, dłuższe "piiiiik"
  buzzerManager.processCommand("{\"state\": \"ON\"}");
  delay(400);  // długość trzeciego sygnału
  buzzerManager.processCommand("{\"state\": \"OFF\"}");
}

// Pomocnicza funkcja generująca jedno długie, niskie piknięcie (Rozbrojenie)
void playDisarmSound() {
  buzzerManager.processCommand("{\"state\": \"ON\"}");
  delay(400);  // jedno dłuższe piknięcie sygnalizujące wyłączenie
  buzzerManager.processCommand("{\"state\": \"OFF\"}");
}

void clearAllAlarms() {
  isSystemArmed = false;
  intrusionAlarmActive = false;
  wrongCardAlarm = false;
  waterAlarmActive = false;
  waterSilenced = false;
  gasAlarmActive = false;
  gasSilenced = false;

  playDisarmSound();

  buzzerManager.processCommand("{\"state\": \"OFF\"}");
  ledManager.setAlarmMode(NONE);
  refreshLights();
  displayManager.clearAlarm();

  connectionManager.publishMessage("makieta/access/status", "DISARMED");
  connectionManager.publishMessage("makieta/sensors", "WATER_OK");
  connectionManager.publishMessage("makieta/sensors", "GAS_OK");

  sendSystemStatus();
}

// --- FUNKCJA RAPORTUJĄCA STAN SYSTEMU DO APLIKACJI MOBILNEJ ---
void sendSystemStatus() {
  String statusLight = "OFF";
  if (ledManager.getWewState() == "ON" || ledManager.getZewState() == "ON") {
    statusLight = "ON";
  }

  String statusVent = (fanManager.getSpeed() > 0) ? "ON" : "OFF";

  String statusSecurity = isSystemArmed ? "ARMED" : "DISARMED";
  if (intrusionAlarmActive || wrongCardAlarm) {
    statusSecurity = "ALARM";
  }

  DynamicJsonDocument statusDoc(256);
  statusDoc["status_light"] = statusLight;
  statusDoc["status_vent"] = statusVent;
  statusDoc["status_security"] = statusSecurity;
  statusDoc["status_heating"] = "OFF";  // Miejsce na przyszłą integrację
  statusDoc["status_auto_light"] = isAutoLightModeEnabled ? "ON" : "OFF";

  String statusJsonStr;
  serializeJson(statusDoc, statusJsonStr);
  connectionManager.publishMessage("makieta/status/systemy", statusJsonStr);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) message += (char)payload[i];
  message.trim();

  String topicStr = String(topic);
  topicStr.trim();

  if (topicStr == "makieta/serwo/ustaw") {
    servoManager.processCommand(message);
    blindsManager.processCommand(message);
  } else if (topicStr == "makieta/sensors/ustaw") {
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
  } else if (topicStr == "makieta/access/ustaw") {
    if (message == "unlock") {
      servoManager.openDoor();
      displayManager.showRfidMessage("Dostep", 2000);
    } else if (message == "ARM") {
      isSystemArmed = true;
      playArmSound();
      buzzerManager.triggerBeep(500);
      connectionManager.publishMessage("makieta/access/status", "ARMED");
    } else if (message == "DISARM") {

      clearAllAlarms();
    }
  } else if (topicStr == "makieta/oswietlenie/ustaw") {
    ledManager.processCommand(message);
    sendSystemStatus();  // <-- DODANE
  } else if (topicStr == "makieta/wentylator/ustaw") {
    fanManager.processCommand(message);
    sendSystemStatus();  // <-- DODANE
  } else if (topicStr == "makieta/buzzer/ustaw") {
    buzzerManager.processCommand(message);
  } else if (topicStr == "makieta/oswietlenie/automatyka") {
    if (message == "ON") {
      isAutoLightModeEnabled = true;
      Serial.println("[INFO] Tryb automatyczny (dzien/noc) WLACZONY");
    } else if (message == "OFF") {
      isAutoLightModeEnabled = false;
      Serial.println("[INFO] Tryb automatyczny (dzien/noc) WYLACZONY - sterowanie reczne");
    }
    sendSystemStatus();  // <-- DODANE
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n==================================================");
  Serial.println("       START SYSTEMU SMART HOME (ESP32)           ");
  Serial.println("==================================================");

  // --- 1. WI-FI ---
  Serial.println("\n[1] INICJALIZACJA WI-FI...");
  connectionManager.initWiFi();
  Serial.print(" -> Polaczono z siecia:  ");
  Serial.println(WiFi.SSID());
  Serial.print(" -> Adres IP ESP32:      ");
  Serial.println(WiFi.localIP());

  // --- 2. MQTT ---
  Serial.println("\n[2] INICJALIZACJA BROKERA MQTT...");
  Serial.print(" -> Adres IP Malinki:    ");
  Serial.println(MQTT_SERVER);
  connectionManager.setupMQTT(MQTT_SERVER, MQTT_PORT);
  connectionManager.setCallback(mqttCallback);

  // --- 3. I2C ORAZ CZUJNIKI ---
  Serial.println("\n[3] INICJALIZACJA CZUJNIKOW I MODULOW...");
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
  blindsManager.init();
  indoorEnvManager.init();

  // --- 4. EKRAN OLED ---
  Serial.println("\n[4] INICJALIZACJA EKRANU OLED...");
  displayManager.init(WiFi.localIP().toString());

  Serial.println("\n==================================================");
  Serial.println("               *** SYSTEM READY *** ");
  Serial.println("==================================================\n");
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
  indoorEnvManager.loop();
  blindsManager.loop();

  // --- HEARTBEAT ---
  static unsigned long lastAppUpdate = 0;
  if (millis() - lastAppUpdate > 5000) {
    lastAppUpdate = millis();

    sendSystemStatus();

    String statusLight = "OFF";
    if (ledManager.getWewState() == "ON" || ledManager.getZewState() == "ON") {
      statusLight = "ON";
    }

    String statusVent = (fanManager.getSpeed() > 0) ? "ON" : "OFF";

    String statusSecurity = isSystemArmed ? "ARMED" : "DISARMED";
    if (intrusionAlarmActive) {
      statusSecurity = "ALARM";
    }

    DynamicJsonDocument statusDoc(256);
    statusDoc["status_light"] = statusLight;
    statusDoc["status_vent"] = statusVent;
    statusDoc["status_security"] = statusSecurity;
    statusDoc["status_heating"] = "OFF";
    statusDoc["status_auto_light"] = isAutoLightModeEnabled ? "ON" : "OFF";

    String statusJsonStr;
    serializeJson(statusDoc, statusJsonStr);
    connectionManager.publishMessage("makieta/status/systemy", statusJsonStr);

    connectionManager.publishMessage("makieta/sensors", waterAlarmActive ? "WATER_ALARM" : "WATER_OK");
    connectionManager.publishMessage("makieta/sensors", gasAlarmActive ? "GAS_ALARM" : "GAS_OK");
    connectionManager.publishMessage("makieta/czujniki/srodowisko/zew", envManager.getSensorJson());
    connectionManager.publishMessage("makieta/czujniki/srodowisko/wew", indoorEnvManager.getSensorJson());
    connectionManager.publishMessage("makieta/czujniki/swiatlo", lightManager.getSensorJson());
    connectionManager.publishMessage("makieta/status/systemy", statusJsonStr);

    Serial.println("\n------------- AKTUALIZACJA DANYCH --------------");
    Serial.print(" Odczyt swiatla:  ");
    Serial.print(lightManager.getLux(), 1);
    Serial.println(" lx");

    Serial.print(" Wnetrze:         ");
    Serial.print(indoorEnvManager.getTemp(), 1);
    Serial.print(" *C,  ");
    Serial.print(indoorEnvManager.getHum(), 1);
    Serial.print(" %,  ");
    Serial.print(indoorEnvManager.getPress(), 1);
    Serial.println(" hPa");

    Serial.print(" Zewnatrz:        ");
    Serial.print(envManager.getTemp(), 1);
    Serial.print(" *C,  ");
    Serial.print(envManager.getHum(), 1);
    Serial.println(" %");

    Serial.print(" Aktywny tryb:    ");
    Serial.println(isNightMode ? "NOC" : "DZIEN");
    Serial.println("------------------------------------------------");
  }

  // --- ALGORYTM DZIEŃ / NOC ---
  static unsigned long lastLightControlTime = 0;
  if (millis() - lastLightControlTime > 1000) {
    lastLightControlTime = millis();
    float currentLux = lightManager.getLux();

    const float LUX_THRESHOLD_NIGHT = 100.0;
    const float LUX_THRESHOLD_DAY = 120.0;

    if (currentLux < LUX_THRESHOLD_NIGHT) {
      if (!isNightMode) {
        isNightMode = true;
        blindsManager.closeBlinds();
        connectionManager.publishMessage("makieta/system/tryb", "NIGHT");
      }

      if (isAutoLightModeEnabled) {
        int autoBrightness = map((long)currentLux, LUX_THRESHOLD_NIGHT, 0, 50, 255);
        autoBrightness = constrain(autoBrightness, 50, 255);

        String jsonCommand = "{\"target\": \"zew\", \"state\": \"ON\", \"brightness\": " + String(autoBrightness) + ", \"color\": {\"r\": 255, \"g\": 255, \"b\": 255}}";
        ledManager.processCommand(jsonCommand);
      }

    } else if (currentLux > LUX_THRESHOLD_DAY) {
      if (isNightMode) {
        isNightMode = false;
        blindsManager.openBlinds();
        connectionManager.publishMessage("makieta/system/tryb", "DAY");

        if (isAutoLightModeEnabled) {
          ledManager.processCommand("{\"target\": \"zew\", \"state\": \"OFF\"}");
        }
      }
    }

    displayManager.updateLighting((int)currentLux, ledManager.getWewState(), ledManager.getZewState());
  }

  // --- LOGIKA CZUJNIKA ZALANIA ---
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

  // --- LOGIKA CZUJNIKA GAZU ---
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

  // --- LOGIKA CZUJNIKA RUCHU ---
  if (motionManager.loop()) {
    Serial.println("\n[ALARM] >>> WYKRYTO RUCH W BUDYNKU! <<<");
    connectionManager.publishMessage("makieta/motion", "MOTION_DETECTED");
    if (isSystemArmed) {
      if (!intrusionAlarmActive) {
        intrusionAlarmActive = true;
        connectionManager.publishMessage("makieta/alarm/intrusion", "ACTIVE");
        buzzerManager.processCommand("{\"state\": \"ON\"}");
        ledManager.setAlarmMode(INTRUSION_ALARM);
        displayManager.triggerAlarm("WYKRYTO RUCH!");
      }
    }
  }

  // --- AKTUALIZACJA WYŚWIETLACZA OLED ---
  if (millis() - lastDisplayUpdate >= displayUpdateInterval) {
    lastDisplayUpdate = millis();
    displayManager.updateSystemState(isSystemArmed);
    displayManager.updateEnvironment(envManager.getTemp(), envManager.getHum(), fanManager.getSpeed());
  }

  // --- LOGIKA ALARMU WŁAMANIOWEGO (PIR) ---
  if (motionManager.loop()) {
    Serial.println("\n[ALARM] >>> WYKRYTO RUCH W BUDYNKU! <<<");
    connectionManager.publishMessage("makieta/motion", "MOTION_DETECTED");

    if (isSystemArmed && !intrusionAlarmActive) {
      intrusionAlarmActive = true;
      
      connectionManager.publishMessage("makieta/alarm/intrusion", "ACTIVE");
      connectionManager.publishMessage("makieta/access/status", "ALARM");
      
      buzzerManager.processCommand("{\"state\": \"ON\"}"); 
      ledManager.setAlarmMode(INTRUSION_ALARM);
      displayManager.triggerAlarm("WYKRYTO RUCH!");
      
      // Natychmiastowa aktualizacja interfejsu (zmiana na ALARM)
      sendSystemStatus();
    }
  }

  // --- LOGIKA DOSTĘPU RFID ---
  if (rfidManager.loop()) {
    String scannedUID = rfidManager.getUID();
    lastCardSeenTime = millis();

    if (!isCardHeld) {
      cardStartTime = millis();
      isCardHeld = true;
      actionExecuted = false;

      Serial.print("\n[ZDARZENIE] >>> UZYTO KARTY RFID. UID: ");
      Serial.print(scannedUID);
      Serial.println(" <<<");

      connectionManager.publishMessage("makieta/access/status", scannedUID);
    }

    if (scannedUID == AUTHORIZED_CARD_1) {
      if (intrusionAlarmActive || wrongCardAlarm) clearAllAlarms();

      if (!actionExecuted && millis() - cardStartTime >= armDelay) {
        isSystemArmed = !isSystemArmed;
        actionExecuted = true;
        connectionManager.publishMessage("makieta/access/status", isSystemArmed ? "ARMED" : "DISARMED");
        if (isSystemArmed) playArmSound();  // Zamiast dawnego buzzerManager.triggerBeep(500);
        else clearAllAlarms();

        sendSystemStatus();
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
      if ((lastCardSeenTime - cardStartTime) < armDelay && !actionExecuted && !isSystemArmed) {
        servoManager.openDoor();
      }
      isCardHeld = false;
    }
  }

  // --- LOGIKA DZWONKA ---
  if (doorbellManager.isRinging()) buzzerManager.triggerDoorbell();
}