#include "ConnectionManager.h"
#include "Config.h"
#include <Arduino.h>

ConnectionManager::ConnectionManager() {
    mqttClient.setClient(espClient);
}

// void ConnectionManager::connectWiFi(const char* ssid, const char* password) {
//     Serial.print("Laczenie z Wi-Fi: ");
//     Serial.println(ssid);
//     WiFi.begin(ssid, password);

//     while (WiFi.status() != WL_CONNECTED) {
//         delay(500);
//         Serial.print(".");
//     }
//     Serial.println("\nPolaczono z Wi-Fi!");
//     Serial.print("Adres IP ESP32: ");
//     Serial.println(WiFi.localIP());
// }

void ConnectionManager::setupMQTT(const char* server, int port) {
    mqttClient.setServer(server, port);
}

void ConnectionManager::setCallback(MQTT_CALLBACK_SIGNATURE) {
    mqttClient.setCallback(callback);
}

void ConnectionManager::reconnectMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Proba polaczenia z brokerem MQTT... ");
        
        String clientId = "ESP32_Makieta-";
        clientId += String(random(0xffff), HEX);

        if (mqttClient.connect(clientId.c_str())) {
            Serial.println("Polaczono z serwerem MQTT na Raspberry Pi!");

            mqttClient.subscribe("makieta/oswietlenie/ustaw");
            Serial.println("Zasubskrybowano temat: makieta/oswietlenie/ustaw");

            // BARKUJĄCA SUBSKRYPCJA BUZZERA (Dodaj to!):
            mqttClient.subscribe("makieta/buzzer/ustaw");
            Serial.println("Zasubskrybowano temat: makieta/buzzer/ustaw");

            mqttClient.subscribe("makieta/wentylator/ustaw");
            Serial.println("Zasubskrybowano temat: makieta/wentylator/ustaw");

            mqttClient.subscribe("makieta/serwo/ustaw");
            Serial.println("Zasubskrybowano temat: makieta/serwo/ustaw");


        } else {
            Serial.print("Blad polaczenia, kod bledu (rc) = ");
            Serial.print(mqttClient.state());
            Serial.println(" Sprobuj ponownie za 5 sekund.");
            delay(5000);
        }
    }
}

void ConnectionManager::loop() {
    if (!mqttClient.connected()) {
        reconnectMQTT();
    }
    mqttClient.loop();
}

bool ConnectionManager::isConnected() {
    return mqttClient.connected();
}

void ConnectionManager::publishMessage(const char* topic, String message) {
    if (mqttClient.connected()) {
        // PubSubClient wymaga tablicy char (c_str), a nie obiektu String
        mqttClient.publish(topic, message.c_str());
    }
}

void ConnectionManager::initWiFi() {
    // Rejestracja dostępnych sieci
    wifiMulti.addAP(WIFI_SSID_1, WIFI_PASS_1);
    wifiMulti.addAP(WIFI_SSID_2, WIFI_PASS_2);

    Serial.println("Nawiązywanie połączenia WiFi...");
    
    // Pętla czekająca na połączenie z dowolną z sieci
    while (wifiMulti.run() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nPołączono z WiFi!");
    Serial.print("Adres IP: ");
    Serial.println(WiFi.localIP());
}