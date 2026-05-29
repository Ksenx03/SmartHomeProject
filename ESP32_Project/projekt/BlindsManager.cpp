#include "BlindsManager.h"

BlindsManager::BlindsManager() {
    isBlindMoving = false;
    blindMoveTime = 0;
}

void BlindsManager::init() {
    // Inicjalizacja kanału LEDC dla pinu 14 (częstotliwość 50Hz, rozdzielczość 12-bit)
    if (ledcAttach(PIN_SERVO_BLIND, 50, 12)) {
        Serial.println("BlindsManager: Inicjalizacja LEDC dla serwa 360 (GPIO 14) udana.");
    }
    // Wymuszenie stanu neutralnego na starcie, aby roleta samoczynnie się nie kręciła
    stopBlinds(); 
}

void BlindsManager::setBlindPWM(int duty) {
    ledcWrite(PIN_SERVO_BLIND, duty);
}

void BlindsManager::openBlinds() {
    // Wypełnienie ok. 410 (impuls ~2.0ms) -> pełna prędkość w jednym kierunku
    setBlindPWM(410);
    isBlindMoving = true;
    blindMoveTime = millis();
    Serial.println("BlindsManager: Otwieranie rolet (ruch w górę)...");
}

void BlindsManager::closeBlinds() {
    // Wypełnienie ok. 205 (impuls ~1.0ms) -> pełna prędkość w przeciwnym kierunku
    setBlindPWM(205);
    isBlindMoving = true;
    blindMoveTime = millis();
    Serial.println("BlindsManager: Zamykanie rolet (ruch w dół)...");
}

void BlindsManager::stopBlinds() {
    // Wypełnienie ok. 307 (impuls ~1.5ms) -> punkt neutralny (zatrzymanie serwa 360)
    setBlindPWM(307);
    isBlindMoving = false;
    Serial.println("BlindsManager: Serwo zatrzymane (stan neutralny).");
}

void BlindsManager::loop() {
    // Asynchroniczne zabezpieczenie przed przeciążeniem i zerwaniem makiety rolety
    if (isBlindMoving && (millis() - blindMoveTime >= blindMoveDuration)) {
        stopBlinds();
    }
}

void BlindsManager::processCommand(String jsonCommand) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonCommand);
    if (error) {
        Serial.print("BlindsManager: Blad parsowania JSON: ");
        Serial.println(error.c_str());
        return;
    }

    // Sprawdzenie czy komenda jest kierowana do tego menedżera
    if (doc.containsKey("target") && doc["target"].as<String>() == "zaluzja") {
        if (doc.containsKey("state")) {
            String state = doc["state"].as<String>();
            if (state == "OPEN") openBlinds();
            else if (state == "CLOSE") closeBlinds();
            else if (state == "STOP") stopBlinds();
        }
    }
}