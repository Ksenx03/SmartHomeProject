#include "BuzzerManager.h"

BuzzerManager::BuzzerManager() {
  isBeeping = false;
  isDingDong = false;
  beepEndTime = 0;
  nextToneTime = 0;
}

void BuzzerManager::init() {
  pinMode(PIN_BUZZER, OUTPUT);
  noTone(PIN_BUZZER);
  digitalWrite(PIN_BUZZER, LOW);
}

void BuzzerManager::setBuzzerState(bool state) {
  if (state) {
    tone(PIN_BUZZER, 2000);
  } else {
    noTone(PIN_BUZZER);
    digitalWrite(PIN_BUZZER, LOW);
  }
}

void BuzzerManager::triggerBeep(unsigned int durationMs) {
  setBuzzerState(true);
  isBeeping = true;
  isDingDong = false;
  beepEndTime = millis() + durationMs;
}

void BuzzerManager::triggerDoorbell() {
  isBeeping = true;
  isDingDong = true;
  tone(PIN_BUZZER, 988);
  nextToneTime = millis() + 300;
  beepEndTime = millis() + 800;
}

void BuzzerManager::processCommand(String jsonCommand) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonCommand);
  if (error) return;

  if (doc.containsKey("action") && doc["action"] == "doorbell") {
    triggerDoorbell();
  }

  if (doc.containsKey("state")) {
    bool state = (doc["state"] == "ON");
    if (state) {
      setBuzzerState(true);
    } else {
      setBuzzerState(false);
      isBeeping = false;
      isDingDong = false;
    }
  }

  if (doc.containsKey("action") && doc["action"] == "beep") {
    unsigned int duration = doc.containsKey("duration") ? doc["duration"] : 500;
    triggerBeep(duration);
  }
}

void BuzzerManager::loop() {
  if (!isBeeping) return;

  unsigned long now = millis();

  if (isDingDong) {
    if (now >= nextToneTime && now < beepEndTime) {
      tone(PIN_BUZZER, 784);
    }
  }

  if (now >= beepEndTime) {
    setBuzzerState(false);
    isBeeping = false;
    isDingDong = false;
  }
}