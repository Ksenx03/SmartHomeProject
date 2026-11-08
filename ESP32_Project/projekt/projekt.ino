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
#include "ClimateManager.h"

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
ClimateManager climateManager(16);

unsigned long lastDisplayUpdate = 0;
const unsigned long displayUpdateInterval = 2000;

void refreshLights() {
  ledManager.processCommand("{\"target\": \"wew\", \"state\": \"" + ledManager.getWewState() + "\"}");
  ledManager.processCommand("{\"target\": \"zew\", \"state\": \"" + ledManager.getZewState() + "\"}");
}

void playArmSound() {
  buzzerManager.processCommand("{\"state\": \"ON\"}");
  delay(120);
  buzzerManager.processCommand("{\"state\": \"OFF\"}");
  delay(80);
  buzzerManager.processCommand("{\"state\": \"ON\"}");
  delay(120);
  buzzerManager.processCommand("{\"state\": \"OFF\"}");
  delay(80);
  buzzerManager.triggerBeep(400);  // Nie blokuje pętli na kolejne 400ms
}

void playDisarmSound() {
  buzzerManager.triggerBeep(400);
}

// void sendSystemStatus() {
//   String statusLight = (ledManager.getWewState() == "ON" || ledManager.getZewState() == "ON") ? "ON" : "OFF";
//   String statusVent = (fanManager.getSpeed() > 0) ? "ON" : "OFF";
//   String statusHeating = climateManager.getIsActive() ? "ON" : "OFF";
//   String statusSecurity = isSystemArmed ? "ARMED" : "DISARMED";

//   if (intrusionAlarmActive || wrongCardAlarm) {
//     statusSecurity = "ALARM";
//   }

//   JsonDocument statusDoc;
//   statusDoc["status_light"] = statusLight;
//   statusDoc["status_vent"] = statusVent;
//   statusDoc["status_security"] = statusSecurity;
//   statusDoc["status_heating"] = statusHeating;
//   statusDoc["status_auto_light"] = isAutoLightModeEnabled ? "ON" : "OFF";

//   String statusJsonStr;
//   serializeJson(statusDoc, statusJsonStr);
//   connectionManager.publishMessage("makieta/status/systemy", statusJsonStr);
// }

void sendSystemStatus() {
  String statusLight = (ledManager.getWewState() == "ON" || ledManager.getZewState() == "ON") ? "ON" : "OFF";
  String statusVent = (fanManager.getSpeed() > 0) ? "ON" : "OFF";
  String statusHeating = climateManager.getIsActive() ? "ON" : "OFF";
  String statusSecurity = isSystemArmed ? "ARMED" : "DISARMED";
  if (intrusionAlarmActive || wrongCardAlarm) {
    statusSecurity = "ALARM";
  }

  JsonDocument statusDoc;
  statusDoc["status_light"] = statusLight;
  statusDoc["status_vent"] = statusVent;
  statusDoc["fan_speed"] = fanManager.getSpeed();
  statusDoc["status_security"] = statusSecurity;
  statusDoc["status_heating"] = statusHeating;
  statusDoc["heating_mode"] = climateManager.isAutoMode() ? "AUTO" : "MANUAL";
  statusDoc["target_temp"] = serialized(String(climateManager.getTargetTemp(), 2));
  statusDoc["blinds_pos"] = blindsManager.getCurrentPosition();
  statusDoc["status_auto_light"] = isAutoLightModeEnabled ? "ON" : "OFF";

  String statusJsonStr;
  serializeJson(statusDoc, statusJsonStr);
  connectionManager.publishMessage("makieta/status/systemy", statusJsonStr);

  // Równocześnie publikujemy status rolet na dedykowanym temacie dla pełnej synchronizacji
  publishBlindsStatus(blindsManager.getCurrentPosition());
}

void clearAllAlarms() {
  isSystemArmed = false;
  intrusionAlarmActive = false;
  wrongCardAlarm = false;
  waterAlarmActive = false;
  waterSilenced = false;
  gasAlarmActive = false;
  gasSilenced = false;

  buzzerManager.processCommand("{\"state\": \"OFF\"}");
  playDisarmSound();
  ledManager.setAlarmMode(NONE);
  refreshLights();
  displayManager.updateSystemState(false);
  displayManager.clearAlarm();

  connectionManager.publishMessage("makieta/access/status", "DISARMED");
  connectionManager.publishMessage("makieta/sensors", "WATER_OK");
  connectionManager.publishMessage("makieta/sensors", "GAS_OK");
  sendSystemStatus();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
  message.trim();

  String topicStr = String(topic);
  topicStr.trim();

  if (topicStr == "makieta/sterowanie/ogrzewanie/temperatura") {
    float temp = message.toFloat();
    if (temp > 0) {
      climateManager.setTargetTemp(temp);
      // Natychmiast przelicz stan przekaźnika względem obecnej temperatury w pokoju
      climateManager.update(indoorEnvManager.getTemp());
      sendSystemStatus();
    }
  } else if (topicStr == "makieta/sterowanie/ogrzewanie/stan") {
    String msgUpper = message;
    msgUpper.toUpperCase();
    if (msgUpper == "AUTO") {
      climateManager.setAutoMode(true);
      climateManager.update(indoorEnvManager.getTemp());
    } else if (msgUpper == "ON") {
      climateManager.setActive(true);
    } else if (msgUpper == "OFF") {
      climateManager.setActive(false);
    }
    sendSystemStatus();
  } else if (topicStr == "makieta/serwo/ustaw") {
    if (message.indexOf("zaluzja") >= 0) {
      blindsManager.processCommand(message);
    } else {
      servoManager.processCommand(message);
    }
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
      displayManager.showRfidMessage("Dostep (App)", 2000);
    } else if (message == "ARM") {
      isSystemArmed = true;
      displayManager.updateSystemState(true);
      playArmSound();
      connectionManager.publishMessage("makieta/access/status", "ARMED");
      sendSystemStatus();
    } else if (message == "DISARM") {
      clearAllAlarms();
    }
  } else if (topicStr == "makieta/oswietlenie/ustaw") {
    ledManager.processCommand(message);
    sendSystemStatus();
  } else if (topicStr == "makieta/wentylator/ustaw") {
    fanManager.processCommand(message);
    sendSystemStatus();
  } else if (topicStr == "makieta/buzzer/ustaw") {
    buzzerManager.processCommand(message);
  } else if (topicStr == "makieta/oswietlenie/automatyka") {
    if (message == "ON") {
      isAutoLightModeEnabled = true;
      Serial.println("[INFO] Tryb automatyczny oswietlenia WLACZONY");
    } else if (message == "OFF") {
      isAutoLightModeEnabled = false;
      Serial.println("[INFO] Tryb automatyczny oswietlenia WYLACZONY");
    }
    sendSystemStatus();
  }
}

void publishBlindsStatus(int pos) {
  String payload = "{\"target\":\"zaluzja\",\"position\":" + String(pos) + "}";
  connectionManager.publishMessage("makieta/serwo/status", payload.c_str());
  Serial.printf("[MQTT] Opublikowano status zaluzji: %s\n", payload.c_str());
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n==================================================");
  Serial.println("       START SYSTEMU SMART HOME (ESP32)           ");
  Serial.println("==================================================");

  Serial.println("\n[1] INICJALIZACJA WI-FI...");
  connectionManager.initWiFi();
  Serial.print(" -> Polaczono z siecia:  ");
  Serial.println(WiFi.SSID());
  Serial.print(" -> Adres IP ESP32:      ");
  Serial.println(WiFi.localIP());

  Serial.println("\n[2] INICJALIZACJA BROKERA MQTT...");
  Serial.print(" -> Adres IP Brokera:    ");
  Serial.println(MQTT_SERVER);
  connectionManager.setupMQTT(MQTT_SERVER, MQTT_PORT);
  connectionManager.setCallback(mqttCallback);

  Serial.println("\n[3] INICJALIZACJA CZUJNIKOW I MODULOW...");
  ledManager.init();
  buzzerManager.init();
  fanManager.init();
  servoManager.init();
  envManager.init();
  gasManager.init();
  waterManager.init();
  lightManager.init();
  rfidManager.init();
  motionManager.init();
  doorbellManager.init();

  indoorEnvManager.init();
  blindsManager.setPositionCallback(publishBlindsStatus);
  blindsManager.init();
  climateManager.init();

  Serial.println("\n[4] INICJALIZACJA EKRANU OLED...");
  displayManager.init(WiFi.localIP().toString());

  Serial.println("\n==================================================");
  Serial.println("               *** SYSTEM GOTOWY ***              ");
  Serial.println("==================================================\n");
}

void loop() {
  connectionManager.loop();
  buzzerManager.loop();
  servoManager.loop();
  displayManager.loop();
  envManager.loop();
  ledManager.update();
  waterManager.loop();
  gasManager.loop();
  lightManager.loop();
  indoorEnvManager.loop();
  blindsManager.loop();

  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command == "HEAT ON") {
      climateManager.setActive(true);
      Serial.println("[TERMINAL] Ogrzewanie WLACZONE (Przekaznik zwarty)");
    } else if (command == "HEAT OFF") {
      climateManager.setActive(false);
      Serial.println("[TERMINAL] Ogrzewanie WYLACZONE (Przekaznik rozwarty)");
    }
  }

  // --- AKTUALIZACJA TERMOSTATU (BEZ WŁĄCZANIA WENTYLATORA!) ---
  bool prevHeatingState = climateManager.getIsActive();
  climateManager.update(indoorEnvManager.getTemp());
  if (climateManager.getIsActive() != prevHeatingState) {
    // Jeśli termostat sam włączył lub wyłączył grzanie, od razu wyślij status do aplikacji
    sendSystemStatus();
  }

  // Zabezpieczenie przed przegrzaniem powyżej 40 stopni (niezależne od zwykłego włączenia ogrzewania)
  if (indoorEnvManager.getTemp() >= 40.0) {
    fanManager.setSpeed(100);
  }

  // --- RAPORT DIAGNOSTYCZNY NA SERIAL MONITOR I MQTT CO 5 SEKUND ---
  static unsigned long lastAppUpdate = 0;
  if (millis() - lastAppUpdate > 5000) {
    lastAppUpdate = millis();

    sendSystemStatus();
    connectionManager.publishMessage("makieta/sensors", waterAlarmActive ? "WATER_ALARM" : "WATER_OK");
    connectionManager.publishMessage("makieta/sensors", gasAlarmActive ? "GAS_ALARM" : "GAS_OK");
    connectionManager.publishMessage("makieta/czujniki/srodowisko/zew", envManager.getSensorJson());
    connectionManager.publishMessage("makieta/czujniki/srodowisko/wew", indoorEnvManager.getSensorJson());
    connectionManager.publishMessage("makieta/czujniki/swiatlo", lightManager.getSensorJson());

    Serial.println("\n==================== DIAGNOSTYKA SYSTEMU ====================");
    Serial.printf(" [SIEC]       Wi-Fi: %s | MQTT: %s\n",
                  WiFi.status() == WL_CONNECTED ? "OK" : "BRAK",
                  connectionManager.isConnected() ? "POLACZONO" : "ROZLACZONO");
    Serial.printf(" [ KLIMAT ]   Wewnatrz:  %.1f *C | %.1f %% | %.1f hPa\n",
                  indoorEnvManager.getTemp(), indoorEnvManager.getHum(), indoorEnvManager.getPress());
    Serial.printf(" [POGODA]     Zewnatrz:  %.1f *C | %.1f %%\n",
                  envManager.getTemp(), envManager.getHum());
    Serial.printf(" [SWIATLO]    Natezenie: %.1f lx | Tryb: %s | Auto-LED: %s\n",
                  lightManager.getLux(), isNightMode ? "NOC" : "DZIEN", isAutoLightModeEnabled ? "ON" : "OFF");
    Serial.printf(" [OSWIETL.]   LED Wew:   %s (Efekt: %s) | LED Zew: %s\n",
                  ledManager.getWewState().c_str(), ledManager.getWewEffectMode().c_str(), ledManager.getZewState().c_str());
    Serial.printf(" [HVAC]       Ogrzewanie (Peltier): %s (Tryb: %s, Cel: %.1f *C) | Wentylator: %d %%\n",
                  climateManager.getIsActive() ? "ON" : "OFF",
                  climateManager.isAutoMode() ? "AUTO" : "MANUAL",
                  climateManager.getTargetTemp(), fanManager.getSpeed());
    Serial.printf(" [MECHANIKA]  Drzwi: %s | Zaluzje: %d %%\n",
                  servoManager.getIsDoorOpen() ? "OTWARTE" : "ZAMKNIETE", blindsManager.getCurrentPosition());
    Serial.printf(" [CZUJNIKI]   Gaz (MQ2): %d %% (ADC: %d, Alarm: %s)\n",
                  gasManager.getPercentage(), gasManager.getRawValue(), gasAlarmActive ? "TAK!" : "NIE");
    Serial.printf("              Woda:      %d %% (ADC: %d, Alarm: %s)\n",
                  waterManager.getPercentage(), waterManager.getRawValue(), waterAlarmActive ? "TAK!" : "NIE");
    Serial.printf("              Ruch PIR:  %s (Czujnik aktywny: %s)\n",
                  motionManager.isMotion() ? "WYKRYTO!" : "BRAK", motionManager.getIsEnabled() ? "TAK" : "NIE");
    Serial.printf(" [BEZPIECZ.]  System:    %s | Alarm wlamaniowy: %s | Alarm zlej karty: %s\n",
                  isSystemArmed ? "UZBROJONY" : "ROZBROJONY", intrusionAlarmActive ? "AKTYWNY!" : "BRAK", wrongCardAlarm ? "AKTYWNY!" : "NIE");
    Serial.printf(" [RFID]       Ostatnia karta UID: %s\n",
                  rfidManager.getUID() == "" ? "Brak odczytow" : rfidManager.getUID().c_str());
    Serial.println("=============================================================\n");
  }

// --- ALGORYTM DZIEN / NOC ---
  static unsigned long lastLightControlTime = 0;
  static bool lightModeInitialized = false; // Chroni przed ruchem rolet przy starcie ESP32!

  // Czekamy minimum 3 sekundy od startu (millis() > 3000), aż czujnik BH1750 wykona pierwszy prawdziwy pomiar
  if (millis() > 3000 && (millis() - lastLightControlTime > 1000)) {
    lastLightControlTime = millis();
    float currentLux = lightManager.getLux();

    const float LUX_THRESHOLD_NIGHT = 100.0;
    const float LUX_THRESHOLD_DAY = 120.0;

    // Pierwsze uruchomienie po starcie ESP32: tylko ustalamy jaki jest tryb (Dzień czy Noc), BEZ ruszania rolet!
    if (!lightModeInitialized) {
      isNightMode = (currentLux < LUX_THRESHOLD_NIGHT);
      lightModeInitialized = true;
      // Wysyłamy do aplikacji prawdziwą pozycję wczytaną z pamięci NVS:
      blindsManager.publishCurrentState();
      Serial.printf("[SYSTEM] Inicjalizacja trybu Dzien/Noc: %s (Pozycja rolet zachowana: %d%%)\n", 
                    isNightMode ? "NOC" : "DZIEN", blindsManager.getCurrentPosition());
    }

    if (currentLux < LUX_THRESHOLD_NIGHT) {
      if (!isNightMode) {
        isNightMode = true;
        connectionManager.publishMessage("makieta/system/tryb", "NIGHT");
        
        // Zamykamy rolety automatycznie TYLKO jeśli tryb Auto jest włączony
        if (isAutoLightModeEnabled) {
          blindsManager.closeBlinds();
        }
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
        connectionManager.publishMessage("makieta/system/tryb", "DAY");
        
        if (isAutoLightModeEnabled) {
          blindsManager.openBlinds();
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
      Serial.println("[ALARM] >>> WYKRYTO ZALANIE! <<<");
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
      Serial.println("[ALARM] >>> WYKRYTO GAZ / DYM! <<<");
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

  // --- LOGIKA CZUJNIKA RUCHU (PIR) ---
  if (motionManager.loop()) {
    static unsigned long lastMotionPrint = 0;
    if (millis() - lastMotionPrint > 2000) {
      lastMotionPrint = millis();
      Serial.println("[CZUJNIK PIR] >>> WYKRYTO RUCH W BUDYNKU! <<<");
      connectionManager.publishMessage("makieta/motion", "MOTION_DETECTED");
    }
    if (isSystemArmed && !intrusionAlarmActive) {
      intrusionAlarmActive = true;
      Serial.println("[ALARM] >>> ALARM WLAMANIOWY AKTYWOWANY! <<<");
      connectionManager.publishMessage("makieta/alarm/intrusion", "ACTIVE");
      connectionManager.publishMessage("makieta/access/status", "ALARM");
      buzzerManager.processCommand("{\"state\": \"ON\"}");
      ledManager.setAlarmMode(INTRUSION_ALARM);
      displayManager.triggerAlarm("WYKRYTO RUCH!");
      sendSystemStatus();
    }
  }

  // --- AKTUALIZACJA WYSWIETLACZA OLED ---
  if (millis() - lastDisplayUpdate >= displayUpdateInterval) {
    lastDisplayUpdate = millis();
    displayManager.updateSystemState(isSystemArmed);
    displayManager.updateEnvironment(indoorEnvManager.getTemp(), indoorEnvManager.getHum(), fanManager.getSpeed(), climateManager.getIsActive());
  }

  // --- LOGIKA DOSTEPU RFID ---
  if (rfidManager.loop()) {
    String scannedUID = rfidManager.getUID();
    lastCardSeenTime = millis();
    bool isAuthorized = (scannedUID == AUTHORIZED_CARD_1 || scannedUID == AUTHORIZED_CARD_2);

    if (!isCardHeld) {
      cardStartTime = millis();
      isCardHeld = true;
      actionExecuted = false;
      Serial.print("\n[RFID] >>> ZBLIZONO KARTE! UID: ");
      Serial.print(scannedUID);
      Serial.println(isAuthorized ? " (AUTORYZOWANA) <<<" : " (NIEAUTORYZOWANA!) <<<");

      // Krótkie piknięcie potwierdzające odczyt zbliżeniowy
      buzzerManager.triggerBeep(150);
      displayManager.showRfidMessage("UID: " + scannedUID, 2000);
      connectionManager.publishMessage("makieta/access/status", scannedUID);
    }

    if (isAuthorized) {
      if (intrusionAlarmActive || wrongCardAlarm) {
        clearAllAlarms();
        actionExecuted = true;
      }
      if (!actionExecuted && (millis() - cardStartTime >= armDelay)) {
        isSystemArmed = !isSystemArmed;
        actionExecuted = true;
        Serial.printf("[RFID] Przytrzymano karte 5s -> Zmiana stanu systemu na: %s\n", isSystemArmed ? "UZBROJONY" : "ROZBROJONY");

        displayManager.updateSystemState(isSystemArmed);
        displayManager.showRfidMessage(isSystemArmed ? "UZBROJONO!" : "ROZBROJONO!", 2000);
        connectionManager.publishMessage("makieta/access/status", isSystemArmed ? "ARMED" : "DISARMED");

        if (isSystemArmed) {
          playArmSound();
        } else {
          clearAllAlarms();
        }
        sendSystemStatus();
      }
    } else {
      if (!wrongCardAlarm) {
        wrongCardAlarm = true;
        Serial.println("[ALARM RFID] >>> ODMOWA DOSTEPU - NIEAUTORYZOWANA KARTA! <<<");
        connectionManager.publishMessage("makieta/access/status", "DENIED");
        displayManager.triggerAlarm("ODMOWA DOSTEPU!");
        ledManager.setAlarmMode(INTRUSION_ALARM);
        buzzerManager.processCommand("{\"state\": \"ON\"}");
        sendSystemStatus();
      }
    }
  } else {
    // Zwiększono tolerancję z 500ms do 1200ms, aby raport diagnostyczny co 5s nie przerywał trzymania karty
    if (isCardHeld && (millis() - lastCardSeenTime > 1200)) {
      String lastUID = rfidManager.getUID();
      bool isAuthorized = (lastUID == AUTHORIZED_CARD_1 || lastUID == AUTHORIZED_CARD_2);

      if (isAuthorized && (lastCardSeenTime - cardStartTime) < armDelay && !actionExecuted && !isSystemArmed) {
        Serial.println("[RFID] Krotkie przylozenie autoryzowanej karty -> Otwieranie drzwi.");
        servoManager.openDoor();
        displayManager.showRfidMessage("Drzwi otwarte", 2000);
      }
      isCardHeld = false;
    }
  }

  // --- LOGIKA DZWONKA ---
  if (doorbellManager.isRinging()) {
    Serial.println("[DZWONEK] >>> Ktos nacisnal przycisk dzwonka! <<<");
    buzzerManager.triggerDoorbell();
  }
}