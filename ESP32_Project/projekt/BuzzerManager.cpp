#include "BuzzerManager.h"

BuzzerManager::BuzzerManager() {
  isBeeping = false;
  beepEndTime = 0;
}

void BuzzerManager::init() {
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);  // Domyślnie buzzer jest wyłączony
}

void BuzzerManager::setBuzzerState(bool state) {
  digitalWrite(PIN_BUZZER, state ? HIGH : LOW);
}

void BuzzerManager::triggerBeep(unsigned int durationMs) {
  setBuzzerState(true);
  isBeeping = true;
  beepEndTime = millis() + durationMs;  // Obliczamy "w przyszłości", kiedy ma się wyłączyć
}

void BuzzerManager::triggerDoorbell() {
  isBeeping = true;
  isDingDong = true;
  // Pierwszy ton: "Ding" (wysoki - 988Hz / nuta B5)
  tone(PIN_BUZZER, 988);
  nextToneTime = millis() + 300;  // "Ding" trwa 300ms
  beepEndTime = millis() + 800;   // Całość trwa 800ms
}

void BuzzerManager::processCommand(String jsonCommand) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonCommand);
  if (error) return;

  // Obsługa nowej akcji "doorbell" z aplikacji lub MQTT
  if (doc.containsKey("action") && doc["action"] == "doorbell") {
    triggerDoorbell();
  }

  // 1. Zmiana ciągłego stanu (np. włączenie/wyłączenie alarmu pożarowego)
  if (doc.containsKey("state")) {
    bool state = (doc["state"] == "ON");
    setBuzzerState(state);
    isBeeping = false;  // Jeśli włączamy alarm na stałe, anulujemy tryb "chwilowego pikania"
  }

  if (doc.containsKey("state")) {
    bool state = (doc["state"] == "ON");
    if (state) tone(PIN_BUZZER, 2000);  // Stały pisk alarmowy
    else noTone(PIN_BUZZER);
    isBeeping = false;
    isDingDong = false;
  }

  // 2. Obsługa polecenia "piknij na określony czas"
  if (doc.containsKey("action") && doc["action"] == "beep") {
    unsigned int duration = doc.containsKey("duration") ? doc["duration"] : 500;  // Domyślnie 500ms
    triggerBeep(duration);
  }
}

void BuzzerManager::loop() {
  if (!isBeeping) return;

  unsigned long now = millis();

  // Logika dwutonowa "Ding-Dong"
  if (isDingDong) {
    if (now >= nextToneTime && now < beepEndTime) {
      // Drugi ton: "Dong" (niższy - 784Hz / nuta G5)
      tone(PIN_BUZZER, 784);
    }
  }

  // Wyłączenie dźwięku po zakończeniu
  if (now >= beepEndTime) {
    noTone(PIN_BUZZER);     // Wyłącza generator PWM (po Ding-Dong i alarmie)
    setBuzzerState(false);  // DODANE: Zdejmuje stan wysoki (po zwykłym beep)
    isBeeping = false;
    isDingDong = false;
  }
}