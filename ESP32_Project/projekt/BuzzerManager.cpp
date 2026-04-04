#include "BuzzerManager.h"

BuzzerManager::BuzzerManager() {
    isBeeping = false;
    beepEndTime = 0;
}

void BuzzerManager::init() {
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW); // Domyślnie buzzer jest wyłączony
}

void BuzzerManager::setBuzzerState(bool state) {
    digitalWrite(PIN_BUZZER, state ? HIGH : LOW);
}

void BuzzerManager::triggerBeep(unsigned int durationMs) {
    setBuzzerState(true);
    isBeeping = true;
    beepEndTime = millis() + durationMs; // Obliczamy "w przyszłości", kiedy ma się wyłączyć
}

void BuzzerManager::processCommand(String jsonCommand) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonCommand);
    if (error) return;

    // 1. Zmiana ciągłego stanu (np. włączenie/wyłączenie alarmu pożarowego)
    if (doc.containsKey("state")) {
        bool state = (doc["state"] == "ON");
        setBuzzerState(state);
        isBeeping = false; // Jeśli włączamy alarm na stałe, anulujemy tryb "chwilowego pikania"
    }

    // 2. Obsługa polecenia "piknij na określony czas"
    if (doc.containsKey("action") && doc["action"] == "beep") {
        unsigned int duration = doc.containsKey("duration") ? doc["duration"] : 500; // Domyślnie 500ms
        triggerBeep(duration);
    }
}

// Ta metoda musi być ciągle wywoływana w głównej pętli programu!
void BuzzerManager::loop() {
    // Jeśli jesteśmy w trybie "pikania" i obecny czas (millis) przekroczył zaplanowany koniec
    if (isBeeping && millis() >= beepEndTime) {
        setBuzzerState(false); // Wyłącz buzzer
        isBeeping = false;     // Zakończ tryb pikania
    }
}