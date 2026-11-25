#include "BlindsManager.h"

BlindsManager::BlindsManager() {
    isBlindMoving = false;
    blindMoveStartTime = 0;
    requiredMoveDuration = 0;
    startPosition = 0;
    currentPosition = 0;
    targetPosition = 0;
}

void BlindsManager::init() {
    if (ledcAttach(PIN_SERVO_BLIND, 50, 12)) {
        Serial.println("BlindsManager: Inicjalizacja LEDC (GPIO 14) zakonczona sukcesem.");
    }
    
    // Wyłączamy sygnał PWM na starcie - serwo stoi całkowicie nieruchomo
    setBlindPWM(0);
    isBlindMoving = false;

    // Odczyt ostatniej pozycji z pamięci nieulotnej NVS
    prefs.begin("blinds", false);
    currentPosition = prefs.getInt("pos", 0); // Domyślnie 0%, jeśli pamięć była pusta
    currentPosition = constrain(currentPosition, 0, 100);
    startPosition = currentPosition;
    targetPosition = currentPosition;

    Serial.printf("BlindsManager: Wczytano pozycje z pamieci NVS: %d%%\n", currentPosition);
}

void BlindsManager::savePosition() {
    prefs.putInt("pos", currentPosition);
    Serial.printf("BlindsManager: Pozycja %d%% zapisana w NVS.\n", currentPosition);
    publishCurrentState();
}

void BlindsManager::publishCurrentState() {
    if (onPositionChanged != nullptr) {
        onPositionChanged(currentPosition);
    }
}

void BlindsManager::setBlindPWM(int duty) {
    ledcWrite(PIN_SERVO_BLIND, duty);
}

// Przelicza na jakim procencie znajduje się roleta, jeśli zatrzymano ją w trakcie ruchu
void BlindsManager::updatePositionInFlight() {
    if (!isBlindMoving || requiredMoveDuration == 0) return;

    unsigned long elapsed = millis() - blindMoveStartTime;
    if (elapsed >= requiredMoveDuration) {
        currentPosition = targetPosition;
    } else {
        float progress = (float)elapsed / (float)requiredMoveDuration;
        int delta = targetPosition - startPosition;
        currentPosition = startPosition + (int)round(delta * progress);
        currentPosition = constrain(currentPosition, 0, 100);
    }
}

void BlindsManager::setPosition(int target) {
    target = constrain(target, 0, 100);

    // Jeśli roleta właśnie jedzie, najpierw przeliczmy gdzie dokładnie teraz jest!
    if (isBlindMoving) {
        updatePositionInFlight();
        setBlindPWM(0);
        isBlindMoving = false;
    }
    
    if (target == currentPosition) {
        Serial.printf("BlindsManager: Zaluzja jest juz na pozycji %d%%\n", currentPosition);
        savePosition(); // Odśwież status w aplikacji
        return;
    }

    int delta = target - currentPosition;
    requiredMoveDuration = (unsigned long)((abs(delta) / 100.0f) * totalTravelTime);
    
    if (requiredMoveDuration < 50) {
        currentPosition = target;
        savePosition();
        return;
    }

    startPosition = currentPosition;
    targetPosition = target;
    blindMoveStartTime = millis();
    isBlindMoving = true;

    if (delta > 0) {
        // Ruch w dół (zamykanie: 0% -> 100%)
        setBlindPWM(410); // ~2000 us
        Serial.printf("BlindsManager: Ruch w dol: %d%% -> %d%% (%lu ms)\n", currentPosition, target, requiredMoveDuration);
    } else {
        // Ruch w górę (otwieranie: 100% -> 0%)
        setBlindPWM(205); // ~1000 us
        Serial.printf("BlindsManager: Ruch w gore: %d%% -> %d%% (%lu ms)\n", currentPosition, target, requiredMoveDuration);
    }
}

void BlindsManager::openBlinds() {
    setPosition(0);
}

void BlindsManager::closeBlinds() {
    setPosition(100);
}

void BlindsManager::stopBlinds() {
    if (isBlindMoving) {
        updatePositionInFlight();
        isBlindMoving = false;
        setBlindPWM(0); // Odcięcie PWM całkowicie zatrzymuje serwo 360 bez drgań
        savePosition();
        Serial.printf("BlindsManager: Zatrzymano recznie na pozycji %d%%\n", currentPosition);
    } else {
        setBlindPWM(0);
        Serial.println("BlindsManager: Silnik zatrzymany.");
    }
}

void BlindsManager::loop() {
    if (isBlindMoving && (millis() - blindMoveStartTime >= requiredMoveDuration)) {
        isBlindMoving = false;
        setBlindPWM(0); // Wyłączamy impulsy PWM po dojechaniu na miejsce
        currentPosition = targetPosition;
        savePosition();
        Serial.printf("BlindsManager: Ruch zakonczony. Aktualna pozycja: %d%%\n", currentPosition);
    }
}

void BlindsManager::processCommand(String jsonCommand) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonCommand);

    if (error) {
        Serial.print("BlindsManager: Blad JSON: ");
        Serial.println(error.c_str());
        return;
    }

    // Obsługa zarówno z kluczem "target":"zaluzja", jak i bezpośrednich komend
    if (!doc.containsKey("target") || doc["target"].as<String>() == "zaluzja") {
        if (doc.containsKey("sync")) {
            setBlindPWM(0);
            isBlindMoving = false;
            currentPosition = constrain(doc["sync"].as<int>(), 0, 100);
            startPosition = currentPosition;
            targetPosition = currentPosition;
            savePosition();
            Serial.printf("BlindsManager: Zsynchronizowano pozycje na %d%%\n", currentPosition);
            return;
        }

        if (doc.containsKey("position")) {
            int pos = doc["position"].as<int>();
            setPosition(pos);
        } else if (doc.containsKey("state")) {
            String state = doc["state"].as<String>();
            state.toUpperCase();
            
            if (state == "OPEN") openBlinds();
            else if (state == "CLOSE") closeBlinds();
            else if (state == "STOP") stopBlinds();
        }
    }
}