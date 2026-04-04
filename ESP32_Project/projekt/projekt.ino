#include <Arduino.h>
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

// --- TUTAJ WPISZ NUMER SWOJEGO BRELOKA/KARTY ---
// KARTA UID: 62 CB 09 51
// BRELOK UID: C2 49 BC 54
const String AUTHORIZED_CARD_1 = "62CB0951";
const String AUTHORIZED_CARD_2 = "00000000";

bool isSystemArmed = false;  // W domu jesteśmy bezpieczni (rozbrojony)
bool isNightMode = false;    // Flaga pamiętająca, czy mamy oficjalnie "Noc"

//bool isSystemArmed = false;         // Czy alarm na złodzieja jest aktywny?
bool intrusionAlarmActive = false;  // Czy właśnie trwa włamanie?

// Liczniki dla RFID
// Liczniki dla RFID
unsigned long cardStartTime = 0;
bool isCardHeld = false;
const unsigned long armDelay = 5000;
unsigned long lastCardSeenTime = 0;  // <--- NOWA ZMIENNA DO DEBOUNCINGU
bool actionExecuted = false;

// Tworzymy globalną instancję zarządcy połączeń
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

// Zmienna do asynchronicznego przesyłania danych do ekranu
unsigned long lastDisplayUpdate = 0;
const unsigned long displayUpdateInterval = 2000;  // Aktualizujemy dane co 2 sekundy

// FUNKCJA CALLBACK: To ona wywołuje się "magicznie" gdy przyjdzie wiadomość z Basha
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Przyszla wiadomosc na temat: ");
  Serial.println(topic);

  // 1. Zamieniamy surowe bajty (payload) na tekst String
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Tresc wiadomosci: ");
  Serial.println(message);

  String topicStr = String(topic);

  // A. Oświetlenie + MANUAL OVERRIDE
  if (topicStr == "makieta/oswietlenie/ustaw") {

    // Szybki podgląd do JSON-a, żeby sprawdzić, co dokładnie wciśnięto
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);

    if (!error && doc["target"] == "wew") {
      String state = doc["state"].as<String>();
      state.toUpperCase();

      if (state == "ON") {
        Serial.println("RECZNE WLACZENIE SWIATLA: Blokuje PIR!");
        // Odbieramy PIR-owi prawo do wyłączenia światła
        motionManager.setPirControlled(false);
      } else if (state == "OFF") {
        Serial.println("RECZNE WYLACZENIE: Odblokowuje PIR!");
        // Zrzucamy flagę. Dzięki temu, gdy znów się poruszysz,
        // PIR zobaczy, że jest "OFF" i ponownie przejmie kontrolę!
        motionManager.setPirControlled(false);
      }
    }

    // Na koniec przepuszczamy komendę dalej, aby pasek faktycznie się zaświecił/zgasił
    ledManager.processCommand(message);
  }
  // 3. Dodajemy przekierowanie dla buzzera
  else if (String(topic) == "makieta/buzzer/ustaw") {
    buzzerManager.processCommand(message);
  }

  // 3. Przekierowanie komend wentylatora
  else if (String(topic) == "makieta/wentylator/ustaw") {
    fanManager.processCommand(message);
  }
  // 4. Przekierowanie komend serwa (MUSI BYĆ else if)
  else if (String(topic) == "makieta/serwo/ustaw") {
    servoManager.processCommand(message);

  } else if (topicStr == "makieta/czujniki/ustaw") {
    motionManager.processCommand(message);
  }
}

void setup() {
  Serial.begin(115200);

  // Inicjalizacja
  ledManager.init();
  buzzerManager.init();
  fanManager.init();
  servoManager.init();
  envManager.init();
  gasManager.init();
  lightManager.init();
  rfidManager.init();
  motionManager.init();

  displayManager.init(WiFi.localIP().toString());

  // 1. Zestawienie połączenia Wi-Fi
  connectionManager.connectWiFi(WIFI_SSID, WIFI_PASSWORD);
  // 2. Konfiguracja adresu brokera
  connectionManager.setupMQTT(MQTT_SERVER, MQTT_PORT);
  // REJESTRACJA CALLBACKA: Mówimy menedżerowi sieci: "jak coś przyjdzie, odpal funkcję mqttCallback"
  connectionManager.setCallback(mqttCallback);
}

void loop() {
  // Metoda loop z ConnectionManager pilnuje stabilności połączenia
  connectionManager.loop();
  // Wielozadaniowość: pozwalamy buzzerowi sprawdzać czas!
  buzzerManager.loop();
  // Pozwalamy serwom pilnować swoich 5 sekund!
  servoManager.loop();
  // 3. Ciągłe odświeżanie interfejsu (FSM ekranu)
  displayManager.loop();

  envManager.loop();

  ledManager.update();

  motionManager.loop();

  // 4. Aktualizacja statusu sieci (szybko reaguje na utratę połączenia)
  //displayManager.updateNetworkStatus(connectionManager.isConnected(), connectionManager.getIP());
  displayManager.updateNetworkStatus(connectionManager.isConnected(), WiFi.localIP().toString());

  // 4. Obsługa czujnika temp. w tle
  if (envManager.loop()) {
    // Jeśli funkcja zwróciła true (czyli minęło 5s i mamy nowy odczyt),
    // pobieramy JSONa i wysyłamy go na MQTT!
    String jsonPayload = envManager.getSensorJson();
    connectionManager.publishMessage("makieta/czujniki/srodowisko", jsonPayload);
  }

  if (gasManager.loop()) {
    String jsonPayload = gasManager.getSensorJson();
    connectionManager.publishMessage("makieta/czujniki/gaz", jsonPayload);
  }

  if (lightManager.loop()) {
    String jsonPayload = lightManager.getSensorJson();
    connectionManager.publishMessage("makieta/czujniki/swiatlo", jsonPayload);
  }

  if (waterManager.loop()) {
    String jsonPayload = waterManager.getSensorJson();
    connectionManager.publishMessage("makieta/czujniki/woda", jsonPayload);
  }

  // --- CYKLICZNE PRZEKAZYWANIE DANYCH DO EKRANU ---
  if (millis() - lastDisplayUpdate >= displayUpdateInterval) {
    lastDisplayUpdate = millis();

    if (isSystemArmed && !intrusionAlarmActive) {
      // JEŚLI UZBROJONY: Wyświetlamy tylko jeden statyczny komunikat
      // Możesz dodać nową metodę do DisplayManager lub użyć triggerAlarm z innym tekstem
      displayManager.triggerAlarm("ALARM AKTYWNY");
    } else if (!isSystemArmed) {

      displayManager.updateEnvironment(envManager.getTemp(), envManager.getHum(), fanManager.getSpeed());
      displayManager.updateLighting(lightManager.getLux(), ledManager.getWewState(), ledManager.getZewState());

      // Publikacja statusu ruchu na MQTT
      connectionManager.publishMessage("makieta/czujniki/ruch", motionManager.getSensorJson());
      // --- Środowisko ---
      float t = envManager.getTemp();
      float h = envManager.getHum();
      int fan = fanManager.getSpeed();

      displayManager.updateEnvironment(t, h, fan);

      // --- Oświetlenie ---
      int lux = lightManager.getLux();
      String ledWew = ledManager.getWewState();
      String ledZew = ledManager.getZewState();

      displayManager.updateLighting(lux, ledWew, ledZew);
      connectionManager.publishMessage("makieta/czujniki/ruch", motionManager.getSensorJson());
    }
  }
  // --- KONTROLA DOSTĘPU (RFID) i INTEGRACJA Z EKRANEM ---
  // if (rfidManager.loop()) {
  //   String scannedUID = rfidManager.getUID();

  //   // Publikacja logu na MQTT
  //   String jsonPayload = rfidManager.getSensorJson();
  //   connectionManager.publishMessage("makieta/kontrola/rfid", jsonPayload);

  //   if (scannedUID == AUTHORIZED_CARD_1 || scannedUID == AUTHORIZED_CARD_2) {
  //     Serial.println("Autoryzacja pomyślna!");

  //     // Komunikat na ekranie! Na 2 sekundy.
  //     displayManager.showRfidMessage("Dostep przyznany!", 2000);

  //     buzzerManager.triggerBeep(100);
  //     servoManager.openDoor();  // Otwiera na 5s asynchronicznie
  //   } else {
  //     Serial.println("Odmowa dostępu!");

  //     // Komunikat o odmowie
  //     displayManager.showRfidMessage("Odmowa dostepu!", 2000);

  //     buzzerManager.triggerBeep(1000);
  //   }
  // }

  static bool criticalAlarmActive = false;

  if (gasManager.isAlarmActive()) {
    if (!criticalAlarmActive) {  // Wykonaj tylko raz przy wykryciu
      buzzerManager.processCommand("{\"state\": \"ON\"}");
      fanManager.processCommand("{\"state\": \"ON\", \"speed\": 80}");
      ledManager.setAlarmMode(GAS_ALARM);
      displayManager.triggerAlarm("WYCIEK GAZU!");
      criticalAlarmActive = true;
    }
  } else if (waterManager.isAlarmActive()) {
    if (!criticalAlarmActive) {
      buzzerManager.processCommand("{\"state\": \"ON\"}");
      ledManager.setAlarmMode(WATER_ALARM);
      displayManager.triggerAlarm("ZALANIE!");
      criticalAlarmActive = true;
    }
  } else {
    // Jeśli zagrożenie minęło
    if (criticalAlarmActive) {
      buzzerManager.processCommand("{\"state\": \"OFF\"}");
      fanManager.processCommand("{\"state\": \"OFF\"}");  // Lub powrót do auto
      ledManager.setAlarmMode(NONE);
      displayManager.clearAlarm();

      // Przywrócenie ostatniego stanu świateł (zgaszenie po alarmie)
      ledManager.processCommand("{\"target\": \"wew\", \"state\": \"OFF\"}");
      ledManager.processCommand("{\"target\": \"zew\", \"state\": \"OFF\"}");

      criticalAlarmActive = false;
    }
  }


  // =========================================================
  // --- 5. LOGIKA CZUJNIKA RUCHU (KOMFORT I WŁAMANIE) ---
  // =========================================================
  if (motionManager.getIsEnabled()) {
    if (motionManager.isMotion()) {
      if (isSystemArmed) {
        // SCENARIUSZ A: WŁAMANIE! (Tego brakowało!)
        if (!intrusionAlarmActive) {
          intrusionAlarmActive = true;
          buzzerManager.processCommand("{\"state\": \"ON\"}");
          ledManager.setAlarmMode(INTRUSION_ALARM);
          displayManager.triggerAlarm("WLAMANIE!");
          connectionManager.publishMessage("makieta/alarm", "{\"status\": \"INTRUSION\"}");
        }
      } else {
        // SCENARIUSZ B: TRYB KOMFORTU (Jesteśmy w domu)
        if (isNightMode) {
          if (ledManager.getWewState() == "OFF") {
            Serial.println("PIR: Wykryto ruch w nocy! Wlaczam oswietlenie.");
            ledManager.processCommand("{\"target\": \"wew\", \"state\": \"ON\", \"brightness\": 76, \"color\": {\"r\": 255, \"g\": 255, \"b\": 255}}");
            motionManager.setPirControlled(true);
          }
        }
      }
    }

    // --- SPRAWDZENIE TIMEOUTU CZUJNIKA (10 sekund dla lampki nocnej) ---
    if (!isSystemArmed && motionManager.checkTimeout()) {
      Serial.println("PIR: Brak ruchu przez 10s. Gasi swiatlo.");
      ledManager.processCommand("{\"target\": \"wew\", \"state\": \"OFF\"}");
      motionManager.setPirControlled(false);
    }
  }

  // =========================================================
  // --- 5. AUTOMATYKA ZMIERZCHOWA I LOGIKA NOCNA ---
  // =========================================================

  // Statyczna flaga pamiętająca porę dnia (zapobiega ciągłemu wysyłaniu komend w pętli)
  float currentLux = lightManager.getLux();

  // Histereza zmierzchowa: włączamy poniżej 30 lx, wyłączamy powyżej 50 lx
  if (currentLux < 30.0 && !isNightMode) {
    isNightMode = true;
    Serial.println("Zapadl zmrok. Aktywuje systemy nocne.");

    // 1. Włączamy zewnętrzny pasek LED na ~30% mocy (jasność 76) w kolorze białym
    ledManager.processCommand("{\"target\": \"zew\", \"state\": \"ON\", \"brightness\": 76, \"color\": {\"r\": 255, \"g\": 255, \"b\": 255}}");

    // Zgodnie z projektem - tutaj następuje automatyczne zamknięcie rolet
    // servoManager.processCommand("{\"target\": \"zaluzja\", \"state\": \"CLOSE\"}");
  } else if (currentLux > 50.0 && isNightMode) {
    isNightMode = false;
    Serial.println("Nastal swit. Dezaktywuje systemy nocne.");

    // 1. Wyłączamy zewnętrzny pasek LED
    ledManager.processCommand("{\"target\": \"zew\", \"state\": \"OFF\"}");

    // Zgodnie z projektem - otwarcie rolet
    // servoManager.processCommand("{\"target\": \"zaluzja\", \"state\": \"OPEN\"}");
  }

  // =========================================================
  // --- KONTROLA DOSTĘPU (RFID): DRZWI ORAZ ALARM (Tap vs Hold) ---
  // =========================================================
  if (rfidManager.loop()) {
    String scannedUID = rfidManager.getUID();

    // Odświeżamy stoper - karta jest WIDZIANA w tej konkretnej chwili
    lastCardSeenTime = millis();

    if (scannedUID == AUTHORIZED_CARD_1 || scannedUID == AUTHORIZED_CARD_2) {
      if (!isCardHeld) {
        // Zarejestrowanie NOWEGO przyłożenia karty
        cardStartTime = millis();
        isCardHeld = true;
        actionExecuted = false;
        Serial.println("Wykryto karte - zabierz szybko (DRZWI) lub trzymaj 5s (ALARM)...");
      }

      // Sprawdzamy czy od momentu przyłożenia minęło już ciągłe 5 sekund
      if (!actionExecuted && millis() - cardStartTime >= armDelay) {
        isSystemArmed = !isSystemArmed;  // Przełączamy stan
        actionExecuted = true;

        if (isSystemArmed) {
          Serial.println(">>> SYSTEM UZBROJONY <<<");
          displayManager.showRfidMessage("ALARM UZBROJONY", 3000);
          buzzerManager.triggerBeep(500);
          ledManager.processCommand("{\"target\": \"wew\", \"state\": \"OFF\"}");
        } else {
          Serial.println(">>> SYSTEM ROZBROJONY <<<");
          displayManager.showRfidMessage("ALARM WYLACZONY", 3000);
          buzzerManager.triggerBeep(100);
          delay(100);
          buzzerManager.triggerBeep(100);

          //Zawsze czyścimy alarm z wyświetlacza przy rozbrajaniu!
          displayManager.clearAlarm();

          if (intrusionAlarmActive) {
            intrusionAlarmActive = false;
            ledManager.setAlarmMode(NONE);
            buzzerManager.processCommand("{\"state\": \"OFF\"}");
            displayManager.clearAlarm();
          }
        }
      }
    } else {
      // Przypadek dla obcej karty
      if (!isCardHeld) {
        Serial.println("Odmowa dostępu!");
        displayManager.showRfidMessage("Odmowa dostepu!", 2000);
        buzzerManager.triggerBeep(1000);
        isCardHeld = true;
        cardStartTime = millis() + 10000;
        actionExecuted = true;
      }
    }
  } else {
    // KARTY NIE MA W ZASIĘGU (lub czytnik zgubił ramkę)
    // SPRAWDZENIE: Czekamy 500ms od ostatniego dobrego odczytu, żeby mieć 100% pewności, że karta została zabrana
    if (isCardHeld && (millis() - lastCardSeenTime > 500)) {

      // Obliczamy ile łącznie trzymano kartę
      unsigned long holdTime = lastCardSeenTime - cardStartTime;

      // Jeżeli zabrano ją przed upływem 5 sekund -> OTWIERAMY DRZWI
      if (holdTime < armDelay) {
        Serial.println("Krotkie dotkniecie karty -> OTWIERAM DRZWI");
        displayManager.showRfidMessage("Dostep przyznany!", 2000);
        buzzerManager.triggerBeep(100);
        servoManager.openDoor();
      }

      // Dopiero teraz układ "zapomina" o karcie
      isCardHeld = false;
    }
  }
}