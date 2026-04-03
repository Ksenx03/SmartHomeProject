#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <WiFi.h>
#include <PubSubClient.h>

class ConnectionManager {
private:
    WiFiClient espClient;
    PubSubClient mqttClient;
    void reconnectMQTT();

public:
    ConnectionManager();
    void connectWiFi(const char* ssid, const char* password);
    void setupMQTT(const char* server, int port);
    void loop(); 
    bool isConnected();

    void setCallback(MQTT_CALLBACK_SIGNATURE);
    void publishMessage(const char* topic, String message);
};

#endif