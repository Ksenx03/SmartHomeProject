#include "ConnectionManager.h"
#include "Config.h"
#include <Arduino.h>

ConnectionManager::ConnectionManager() {
    mqttClient.setClient(espClient);
}

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
            Serial.println("Polaczono z serwerem MQTT!");

            // --- SUBSKRYPCJE (Nasłuchiwanie komend) ---
            mqttClient.subscribe("makieta/oswietlenie/ustaw");
            mqttClient.subscribe("makieta/buzzer/ustaw");
            mqttClient.subscribe("makieta/wentylator/ustaw");
            mqttClient.subscribe("makieta/serwo/ustaw");
            mqttClient.subscribe("makieta/access/ustaw");
            mqttClient.subscribe("makieta/oswietlenie/automatyka"); 
            
            // ИСПРАВЛЕНО: Теперь топик совпадает с приложением (sensors вместо czujniki)
            mqttClient.subscribe("makieta/sensors/ustaw");

            Serial.println("Wszystkie tematy zasubskrybowane pomyślnie!");

        } else {
            Serial.print("Blad polaczenia, rc = ");
            Serial.print(mqttClient.state());
            Serial.println(" Sprobuj ponownie за 5 секунд.");
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
        mqttClient.publish(topic, message.c_str());
    }
}

void ConnectionManager::initWiFi() {
    wifiMulti.addAP(WIFI_SSID_1, WIFI_PASS_1);
    wifiMulti.addAP(WIFI_SSID_2, WIFI_PASS_2);

    Serial.println("Nawiązywanie połączenia WiFi...");
    
    while (wifiMulti.run() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nPołączono z WiFi!");
    Serial.print("Adres IP: ");
    Serial.println(WiFi.localIP());
}