#include "ServoManager.h"

ServoManager::ServoManager() {
    isDoorOpen = false;
    isServoActive = false;
    doorOpenTime = 0;
    servoMoveStartTime = 0;
}

void ServoManager::init() {
    if (ledcAttach(PIN_SERVO_DOOR, 50, 12)) {
        Serial.println("Servo: Inicjalizacja LEDC (50Hz, 12-bit) udana.");
    }
    closeDoor();
}

void ServoManager::setServoAngle(int angle) {
    angle = constrain(angle, 0, 180);
    int duty = map(angle, 0, 180, 102, 491);
    ledcWrite(PIN_SERVO_DOOR, duty);
    servoMoveStartTime = millis();
    isServoActive = true;
}

void ServoManager::openDoor() {
    if (!isDoorOpen) {
        setServoAngle(150);
        isDoorOpen = true;
        doorOpenTime = millis();
        Serial.println("Zamek: Drzwi OTWARTE");
    } else {
        // Jesli drzwi sa juz otwarte, tylko przedluzamy czas otwarcia bez szarpania serwem
        doorOpenTime = millis();
    }
}

void ServoManager::closeDoor() {
    setServoAngle(0);
    isDoorOpen = false;
    Serial.println("Zamek: Drzwi ZAMKNIETE");
}

bool ServoManager::getIsDoorOpen() {
    return isDoorOpen;
}

void ServoManager::loop() {
    unsigned long now = millis();

    // Po 800 ms od ruchu wyłączamy sygnał PWM, aby serwo nie pobierało prądu i nie zawieszało ESP32
    if (isServoActive && (now - servoMoveStartTime >= servoDetachDelay)) {
        ledcWrite(PIN_SERVO_DOOR, 0);
        isServoActive = false;
    }

    // Automatyczne zamknięcie drzwi po 5 sekundach
    if (isDoorOpen && (now - doorOpenTime >= doorOpenDuration)) {
        closeDoor();
    }
}

void ServoManager::processCommand(String jsonCommand) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonCommand);
    if (error) {
        Serial.print("Servo: Blad JSON: ");
        Serial.println(error.c_str());
        return;
    }

    if (doc.containsKey("angle")) {
        int angle = doc["angle"];
        setServoAngle(angle);
        isDoorOpen = false;
        Serial.printf("Servo: Ustawiono kat %d przez MQTT\n", angle);
    }

    if (doc.containsKey("state")) {
        String state = doc["state"];
        if (state == "OPEN") openDoor();
        else if (state == "CLOSE") closeDoor();
    }
}