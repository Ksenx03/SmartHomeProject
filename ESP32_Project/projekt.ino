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

// --- TUTAJ WPISZ NUMER SWOJEGO BRELOKA/KARTY ---
// KARTA UID: 62 CB 09 51
// BRELOK UID: C2 49 BC 54
const String AUTHORIZED_CARD = "62CB0951";

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

    // 2. Jeśli wiadomość przyszła na odpowiedni temat, przekazujemy ją do LedManager'a!
    if (String(topic) == "makieta/oswietlenie/ustaw") {
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

    // if (rfidManager.loop()) {
    //     // Gdy karta zostanie odczytana:
    //     String jsonPayload = rfidManager.getSensorJson();
        
    //     // Wysyłamy nr karty do brokera
    //     connectionManager.publishMessage("makieta/kontrola/rfid", jsonPayload);
        
    //     // Piszczymy buzzerem na 200ms (sprzężenie zwrotne dla użytkownika!)
    //     buzzerManager.triggerBeep(200);
        
    //     Serial.print("Odczytano karte: ");
    //     Serial.println(rfidManager.getUID());
    // }

    // pętla głowna do RFID i serwo (otwieranie)
    if (rfidManager.loop()) {
        String scannedUID = rfidManager.getUID();
        
        // Zawsze wysyłamy log do brokera MQTT (nawet o błędnych kartach)
        String jsonPayload = rfidManager.getSensorJson();
        connectionManager.publishMessage("makieta/kontrola/rfid", jsonPayload);
        
        // Weryfikacja karty (KONTROLA DOSTĘPU)
        if (scannedUID == AUTHORIZED_CARD) {
            Serial.println("Autoryzacja pomyslna!");
            buzzerManager.triggerBeep(100); // Jedno krótkie piknięcie radości
            servoManager.openDoor();        // Otwieramy serwo!
        } else {
            Serial.println("Odmowa dostepu!");
            buzzerManager.triggerBeep(1000); // Długi dźwięk błędu (1 sekunda)
        }
    }


}