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

// --- ZMIENNE STANU (STATE VARIABLES) ---
const String AUTHORIZED_CARD_1 = "62CB0951"; // Zarejestrowana karta nr 1
const String AUTHORIZED_CARD_2 = "C249BC54"; // Zarejestrowana karta nr 2

bool isSystemArmed = false; // Flaga uzbrojenia alarmu
bool isNightMode = false; // Flaga trybu nocnego
bool intrusionAlarmActive = false; // Flaga aktywnego alarmu włamania (wykrycie ruchu)
bool wrongCardAlarm = false; // Flaga alarmu użycia nieautoryzowanej karty RFID

// Statusy czujników awaryjnych
bool waterAlarmActive = false; // Alarm zalania
bool waterSilenced = false; // Wyciszenie alarmu zalania z poziomu aplikacji
bool gasAlarmActive = false; // Alarm gazu/dymu
bool gasSilenced = false; // Wyciszenie alarmu gazu z poziomu aplikacji

// Timery do filtrowania szumów (tzw. debounce / zapobieganie fałszywym alarmom z czujników)
unsigned long waterTriggerTime = 0;
unsigned long gasTriggerTime = 0;
const unsigned long alarmDelayThreshold = 500; // Czas (ms), po którym sygnał wyzwala faktyczny alarm

// Timery i zmienne dla logiki czytnika RFID
unsigned long cardStartTime = 0; // Czas przyłożenia karty do czytnika
bool isCardHeld = false; // Flaga określająca, czy karta jest trzymana na czytniku
const unsigned long armDelay = 5000; // Czas przytrzymania (5s) wymagany do uzbrojenia/rozbrojenia systemu
unsigned long lastCardSeenTime = 0; // Ostatni moment odczytu (zapobiega "skakaniu" sygnału czytnika)
bool actionExecuted = false; // Zapobiega wielokrotnemu wykonaniu tej samej akcji przy jednym przyłożeniu karty

// Inicjalizacja menedżerów (zarządców) sterujących poszczególnymi elementami makiety
ConnectionManager connectionManager; // Odpowiada za WiFi oraz brokera MQTT
LedStripManager ledManager; // Sterowanie paskami LED (oświetlenie wewnętrzne i zewnętrzne)
BuzzerManager buzzerManager; // Sterowanie głośnikiem/buzzerem (alarmy, dzwonek)
FanManager fanManager; // Sterowanie wentylatorem / wentylacją
EnvironmentManager envManager; // Czujnik środowiskowy (temperatura, wilgotność)
GasSensorManager gasManager; // Czujnik gazu/dymu
LightSensorManager lightManager; // Czujnik natężenia światła (fotorezystor)
WaterSensorManager waterManager; // Czujnik poziomu/obecności wody
RfidManager rfidManager; // Kontrola dostępu - Czytnik RFID
ServoManager servoManager; // Sterowanie serwomechanizmem (zamek do drzwi)
DisplayManager displayManager; // Obsługa wyświetlacza OLED na makiecie
MotionSensorManager motionManager; // Czujnik ruchu (np. PIR)
DoorbellManager doorbellManager; // Przycisk dzwonka do drzwi

unsigned long lastDisplayUpdate = 0; // Timer cyklicznego odświeżania ekranu
const unsigned long displayUpdateInterval = 2000; // Ekran aktualizuje się co 2 sekundy

// --- FUNKCJA WYMUSZONEGO ODŚWIEŻENIA OŚWIETLENIA ---
// Przywraca stan diod LED na podstawie ich ostatnich zapisanych wartości (sprzed wyzwolenia alarmu).
// Przydaje się, gdy zdejmujemy alarm, który tymczasowo "nadpisał" kolory pasków LED.
void refreshLights() {
  ledManager.processCommand("{\"target\": \"wew\", \"state\": \"" + ledManager.getWewState() + "\"}");
  ledManager.processCommand("{\"target\": \"zew\", \"state\": \"" + ledManager.getZewState() + "\"}");
}

// --- RESETOWANIE WSZYSTKICH ALARMÓW ---
// Funkcja przywraca system do "czystego" stanu: wyłącza alarmy, brzęczyk, resetuje wszystkie flagi.
void clearAllAlarms() {
  isSystemArmed = false;
  intrusionAlarmActive = false;
  wrongCardAlarm = false;
  waterAlarmActive = false;
  waterSilenced = false;
  gasAlarmActive = false;
  gasSilenced = false;

  buzzerManager.processCommand("{\"state\": \"OFF\"}"); // Wyłączenie syreny
  ledManager.setAlarmMode(NONE); // Wyłączenie policyjnych/ostrzegawczych efektów LED
  refreshLights(); // Przywrócenie normalnego trybu oświetlenia
  displayManager.clearAlarm(); // Wyczyszczenie komunikatu o alarmie z wyświetlacza

  // Aktualizacja statusów w aplikacji (przesłanie informacji na brokera MQTT)
  connectionManager.publishMessage("makieta/access/status", "DISARMED");
  connectionManager.publishMessage("makieta/sensors", "WATER_OK");
  connectionManager.publishMessage("makieta/sensors", "GAS_OK");
}

// --- CALLBACK (OBSŁUGA KOMEND PRZYCHODZĄCYCH Z APLIKACJI PRZEZ MQTT) ---
// Funkcja wywoływana automatycznie w momencie otrzymania komendy od aplikacji mobilnej (np. z Raspberry).
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) message += (char)payload[i];
  String topicStr = String(topic);

  // 1. Czujniki (np. obsługa awaryjnych przycisków "STOP ALARM" w aplikacji Android)
  if (topicStr == "makieta/sensors/ustaw") {
    if (message == "water_off") {
      // Użytkownik wymusił wyciszenie alarmu wody na telefonie
      waterAlarmActive = false;
      waterSilenced = true;
      buzzerManager.processCommand("{\"state\": \"OFF\"}");
      ledManager.setAlarmMode(NONE);
      refreshLights();
      connectionManager.publishMessage("makieta/sensors", "WATER_OK");
    } else if (message == "gas_off") {
      // Użytkownik wymusił wyciszenie alarmu gazu na telefonie
      gasAlarmActive = false;
      gasSilenced = true;
      buzzerManager.processCommand("{\"state\": \"OFF\"}");
      ledManager.setAlarmMode(NONE);
      refreshLights();
      connectionManager.publishMessage("makieta/sensors", "GAS_OK");
    }
  }
  // 2. Dostęp / Ochrona (obsługa zamka, uzbrajanie i rozbrajanie alarmu)
  else if (topicStr == "makieta/access/ustaw") {
    if (message == "unlock") {
      servoManager.openDoor(); // Otwarcie rygla drzwi
      displayManager.showRfidMessage("Dostep", 2000);
    } else if (message == "ARM") {
      isSystemArmed = true; // Zdalne włączenie alarmu (ochrona przed włamaniem)
      buzzerManager.triggerBeep(500); // Dźwiękowe potwierdzenie
      connectionManager.publishMessage("makieta/access/status", "ARMED");
    } else if (message == "DISARM") {
      clearAllAlarms(); // Zdalne wyłączenie alarmu i zresetowanie flag
    }
  } 
  // 3. Przekazanie komend środowiskowych bezpośrednio do menedżerów
  else if (topicStr == "makieta/oswietlenie/ustaw") ledManager.processCommand(message);
  else if (topicStr == "makieta/wentylator/ustaw") fanManager.processCommand(message);
  else if (topicStr == "makieta/buzzer/ustaw") buzzerManager.processCommand(message);
  else if (topicStr == "makieta/serwo/ustaw") servoManager.processCommand(message);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Inicjalizacja poszczególnych modułów systemu IoT (konfiguracja pinów, bibliotek I2C/SPI itp.)
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
  displayManager.init(WiFi.localIP().toString()); // Inicjalizacja ekranu, opcjonalnie wyświetlenie IP

  // Setup komunikacji sieciowej
  connectionManager.initWiFi();
  connectionManager.setupMQTT(MQTT_SERVER, MQTT_PORT);
  connectionManager.setCallback(mqttCallback);

  // Usunięto problematyczne linie connectionManager.subscribe —
  // subskrypcja tematów MQTT odbywa się teraz automatycznie wewnątrz klasy ConnectionManager.cpp!

  Serial.println("System Ready");
}

void loop() {
  // Ciągłe wywoływanie funkcji loop dla poszczególnych modułów.
  // Umożliwia to wielozadaniowość asynchroniczną (brak użycia funkcji delay(), która blokuje system).
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

  if (millis() - lastLightControlTime > 1000) { // Sprawdzaj co 1 sekundę
    lastLightControlTime = millis();
    float currentLux = lightManager.getLux(); // Pobranie wartości luksów (natężenie światła)

    // Konfiguracja histerezy – uodparnia system na migotanie oświetlenia w okolicach granicy przełączenia
    const float LUX_THRESHOLD_NIGHT = 100.0;
    const float LUX_THRESHOLD_DAY = 120.0;

    // Przejście w tryb NOCNY
    if (currentLux < LUX_THRESHOLD_NIGHT) {
      if (!isNightMode) {
        isNightMode = true;
        connectionManager.publishMessage("makieta/system/tryb", "NIGHT");
      }

      // Dynamiczna regulacja jasności świateł zewn. względem otoczenia (ciemniej = światła jaśniejsze)
      int autoBrightness = map((long)currentLux, LUX_THRESHOLD_NIGHT, 0, 50, 255);
      autoBrightness = constrain(autoBrightness, 50, 255); // Ograniczenie wartości od 50 do 255 PWM

      // Generowanie i wysyłanie JSON z komendą do paska LED
      String jsonCommand = "{\"target\": \"zew\", \"state\": \"ON\", \"brightness\": " + String(autoBrightness) + ", \"color\": {\"r\": 255, \"g\": 255, \"b\": 255}}";
      ledManager.processCommand(jsonCommand);

    // Przejście w tryb DZIENNY
    } else if (currentLux > LUX_THRESHOLD_DAY) {
      if (isNightMode) {
        isNightMode = false;
        connectionManager.publishMessage("makieta/system/tryb", "DAY");

        // Wyłączanie oświetlenia zewnętrznego
        String jsonCommand = "{\"target\": \"zew\", \"state\": \"OFF\"}";
        ledManager.processCommand(jsonCommand);
      }
    }

    // === AKTUALIZACJA WYŚWIETLACZA OLED (Status Oświetlenia) ===
    // Pobieramy faktyczne stany z LedStripManager i przekazujemy na ekran
    displayManager.updateLighting(
      (int)currentLux,
      ledManager.getWewState(),
      ledManager.getZewState());
  }

  // --- LOGIKA CZUJNIKA ZALANIA ---
  if (waterManager.isAlarmActive()) {
    if (waterTriggerTime == 0) waterTriggerTime = millis(); // Moment wykrycia pierwszej kropli
    // Debounce: jeśli sygnał jest stały powyżej zadanego czasu i alarm nie został wyciszony na telefonie
    if (!waterAlarmActive && !waterSilenced && (millis() - waterTriggerTime > alarmDelayThreshold)) {
      waterAlarmActive = true;
      connectionManager.publishMessage("makieta/sensors", "WATER_ALARM");
      buzzerManager.processCommand("{\"state\": \"ON\"}"); // Uruchom alarm dźwiękowy
      ledManager.setAlarmMode(WATER_ALARM); // Odpal efekty wizualne diod
      displayManager.triggerAlarm("WYKRYTO ZALANIE");
    }
  } else {
    waterTriggerTime = 0; // Czyszczenie timera
    if (waterAlarmActive || waterSilenced) { // Automatyczny powrót z alarmu po wysuszeniu czujnika
      waterAlarmActive = false;
      waterSilenced = false;
      connectionManager.publishMessage("makieta/sensors", "WATER_OK");
      // Wyłącz dźwięk i światła TYLKO wtedy, gdy nie trwają jeszcze inne awarie (włamanie/gaz)
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
    // Odfiltrowanie pojedynczych skoków pomiaru z czujnika dymu (debounce)
    if (!gasAlarmActive && !gasSilenced && (millis() - gasTriggerTime > alarmDelayThreshold)) {
      gasAlarmActive = true;
      connectionManager.publishMessage("makieta/sensors", "GAS_ALARM");
      buzzerManager.processCommand("{\"state\": \"ON\"}");
      ledManager.setAlarmMode(GAS_ALARM);
      displayManager.triggerAlarm("WYKRYTO GAZ/DYM");
    }
  } else {
    gasTriggerTime = 0;
    if (gasAlarmActive || gasSilenced) { // Gaz się ulotnił
      gasAlarmActive = false;
      gasSilenced = false;
      connectionManager.publishMessage("makieta/sensors", "GAS_OK");
      // Wyłącz alert jeśli to była jedyna anomalia
      if (!waterAlarmActive && !intrusionAlarmActive) {
        buzzerManager.processCommand("{\"state\": \"OFF\"}");
        ledManager.setAlarmMode(NONE);
        refreshLights();
        displayManager.clearAlarm();
      }
    }
  }

  // // Heartbeat dla aplikacji (stara wysyłka zakomentowana)
  // static unsigned long lastAppUpdate = 0;
  // if (millis() - lastAppUpdate > 5000) { ... }

  // Heartbeat (okresowa wysyłka) dla aplikacji Kotlin (raz na 5 sek)
  static unsigned long lastAppUpdate = 0;
  if (millis() - lastAppUpdate > 5000) {
    lastAppUpdate = millis();
    connectionManager.publishMessage("makieta/sensors", waterAlarmActive ? "WATER_ALARM" : "WATER_OK");
    connectionManager.publishMessage("makieta/sensors", gasAlarmActive ? "GAS_ALARM" : "GAS_OK");

    // TĘ LINIJKĘ DODAJESZ: Publikacja temperatury i wilgotności w skonsolidowanym formacie JSON
    connectionManager.publishMessage("makieta/czujniki/srodowisko", envManager.getSensorJson());

    // (Opcjonalnie) Możesz usunąć tę starą linijkę z "HUM:", bo nowa paczka JSON zawiera już wilgotność
    connectionManager.publishMessage("makieta/sensors", "HUM:" + String((int)envManager.getHum()));

    // Przekazanie wartości luksów na dedykowany temat MQTT
    connectionManager.publishMessage("makieta/czujniki/swiatlo", lightManager.getSensorJson());
  }

  // === AKTUALIZACJA WYŚWIETLACZA OLED ===
  if (millis() - lastDisplayUpdate >= displayUpdateInterval) {
    lastDisplayUpdate = millis();

    // Przekazanie flagi - DisplayManager sam spokojnie włączy ekran "SYSTEM UZBROJONY"
    displayManager.updateSystemState(isSystemArmed);

    // Pobieranie danych o środowisku w tle i rzucanie ich na OLED
    displayManager.updateEnvironment(envManager.getTemp(), envManager.getHum(), fanManager.getSpeed());
  }

  // --- LOGIKA ALARMU WŁAMANIOWEGO (PIR) ---
  if (motionManager.getIsEnabled() && motionManager.isMotion()) {
    if (isSystemArmed && !intrusionAlarmActive) { // Wykryto ruch, a system był uprzednio uzbrojony
      intrusionAlarmActive = true;
      connectionManager.publishMessage("makieta/access/status", "ALARM");
      buzzerManager.processCommand("{\"state\": \"ON\"}"); // Odpal syrenę alarmu
      ledManager.setAlarmMode(INTRUSION_ALARM); // Policyjne miganie LED
      displayManager.triggerAlarm("WYKRYTO RUCH!");
    }
  }

  // --- LOGIKA DOSTĘPU RFID (Zamek oraz Uzbrajanie Systemu) ---
  if (rfidManager.loop()) {
    String scannedUID = rfidManager.getUID();
    lastCardSeenTime = millis(); // Resetujemy czas, w którym karta zniknęła z radaru

    if (!isCardHeld) {
      cardStartTime = millis(); // Oznaczenie, od kiedy karta leży na czytniku
      isCardHeld = true;
      actionExecuted = false; // Gotowość do wykonania akcji po czasie
      connectionManager.publishMessage("makieta/access/status", scannedUID); // Przekazanie do backendu (np. kto wszedł)
    }

    if (scannedUID == AUTHORIZED_CARD_1) {
      if (intrusionAlarmActive || wrongCardAlarm) clearAllAlarms(); // Skasuj włamanie/obcą kartę jeśli autoryzowany wszedł
      
      // Jeżeli użytkownik przytrzyma kartę długo (>5s) następuje zmiana statusu alarmu
      if (!actionExecuted && millis() - cardStartTime >= armDelay) {
        isSystemArmed = !isSystemArmed; // Toggle (Uzbrój / Rozbrój)
        actionExecuted = true; // Blokada przed wielokrotnym przeliczaniem podczas jednego przyłożenia
        connectionManager.publishMessage("makieta/access/status", isSystemArmed ? "ARMED" : "DISARMED");
        
        if (isSystemArmed) buzzerManager.triggerBeep(500); // 1 piknięcie na uzbrojenie
        else clearAllAlarms(); // Czyszczenie całego syfu przy rozbrojeniu
      }
    } else {
      // Przyłożono "obcą" kartę do czytnika
      if (!wrongCardAlarm) {
        wrongCardAlarm = true;
        connectionManager.publishMessage("makieta/access/status", "DENIED");
        ledManager.setAlarmMode(INTRUSION_ALARM); // Opcjonalny wizualny odstraszacz włamywacza
        buzzerManager.processCommand("{\"state\": \"ON\"}");
      }
    }
  } else {
    // Logika puszczenia (odciągnięcia) karty. Jeżeli odcięto sygnał na ponad 500ms...
    if (isCardHeld && (millis() - lastCardSeenTime > 500)) {
      // Jeżeli trzymano krócej niż 5s (brak akcji systemowej), system nie był uzbrojony
      // -> Traktujemy to jako chęć szybkiego otwarcia drzwi.
      if ((lastCardSeenTime - cardStartTime) < armDelay && !actionExecuted && !isSystemArmed) servoManager.openDoor();
      
      isCardHeld = false; // Reset trzymania
    }
  }

  // --- LOGIKA DZWONKA ---
  if (doorbellManager.isRinging()) buzzerManager.triggerDoorbell(); // Wywołaj melodię z buzzera
}