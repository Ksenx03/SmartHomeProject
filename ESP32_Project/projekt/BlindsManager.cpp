#include "BlindsManager.h"

BlindsManager::BlindsManager() {
    isBlindMoving = false;
    blindMoveStartTime = 0;
    requiredMoveDuration = 0;
    currentPosition = 100; // Стартуем со 100% (штора опущена)
    targetPosition = 100;
}

void BlindsManager::init() {
    if (ledcAttach(PIN_SERVO_BLIND, 50, 12)) {
        Serial.println("BlindsManager: Инициализация LEDC (GPIO 14) успешна.");
    }
    stopBlinds();

    // Загрузка положения из Flash-памяти (по умолчанию 100%)
    prefs.begin("blinds", false);
    currentPosition = prefs.getInt("pos", 100);
    targetPosition = currentPosition;
    savePosition();
    Serial.printf("BlindsManager: Загружено положение: %d%%\n", currentPosition);
}

void BlindsManager::savePosition() {
    prefs.putInt("pos", currentPosition);
    Serial.printf("BlindsManager: Положение %d%% сохранено в NVS.\n", currentPosition);

    if (onPositionChanged != nullptr) {
        onPositionChanged(currentPosition);
    }
}

void BlindsManager::setBlindPWM(int duty) {
    ledcWrite(PIN_SERVO_BLIND, duty);
}

void BlindsManager::setPosition(int target) {
    target = constrain(target, 0, 100);
    
    if (target == currentPosition) {
        Serial.printf("BlindsManager: Штора уже на %d%%\n", currentPosition);
        return;
    }

    int delta = target - currentPosition;
    requiredMoveDuration = (unsigned long)((abs(delta) / 100.0f) * totalTravelTime);
    
    if (requiredMoveDuration < 50) {
        currentPosition = target;
        savePosition();
        return;
    }

    targetPosition = target;
    blindMoveStartTime = millis();
    isBlindMoving = true;

    if (delta > 0) {
        // Закрытие (движение вниз) -> ШИМ 410
        setBlindPWM(410);
        Serial.printf("BlindsManager: Вниз: %d%% -> %d%% (%lu мс)\n", currentPosition, target, requiredMoveDuration);
    } else {
        // Открытие (движение вверх) -> ШИМ 205
        setBlindPWM(205);
        Serial.printf("BlindsManager: Вверх: %d%% -> %d%% (%lu мс)\n", currentPosition, target, requiredMoveDuration);
    }
}

void BlindsManager::openBlinds() {
    setPosition(0);
}

void BlindsManager::closeBlinds() {
    setPosition(100);
}

void BlindsManager::stopBlinds() {
    setBlindPWM(307); // Нейтраль (остановка мотора)
    isBlindMoving = false;
    Serial.println("BlindsManager: Мотор остановлен.");
}

void BlindsManager::loop() {
    if (isBlindMoving && (millis() - blindMoveStartTime >= requiredMoveDuration)) {
        stopBlinds();
        currentPosition = targetPosition;
        savePosition();
        Serial.printf("BlindsManager: Движение завершено. Позиция: %d%%\n", currentPosition);
    }
}

void BlindsManager::processCommand(String jsonCommand) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonCommand);
    if (error) {
        Serial.print("BlindsManager: Ошибка JSON: ");
        Serial.println(error.c_str());
        return;
    }

    if (doc.containsKey("target") && doc["target"].as<String>() == "zaluzja") {
        // Принудительная синхронизация положения без движения мотора
        if (doc.containsKey("sync")) {
            currentPosition = constrain(doc["sync"].as<int>(), 0, 100);
            targetPosition = currentPosition;
            savePosition();
            stopBlinds();
            Serial.printf("BlindsManager: Синхронизировано на %d%%\n", currentPosition);
            return;
        }

        // Позиция в процентах (0..100) от кнопок или слайдера
        if (doc.containsKey("position")) {
            int pos = doc["position"].as<int>();
            setPosition(pos);
        } 
        // Текстовые команды
        else if (doc.containsKey("state")) {
            String state = doc["state"].as<String>();
            state.toUpperCase();
            
            if (state == "OPEN") openBlinds();
            else if (state == "CLOSE") closeBlinds();
            else if (state == "STOP") stopBlinds();
        }
    }
}