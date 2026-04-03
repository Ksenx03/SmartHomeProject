#include "ServoManager.h"

ServoManager::ServoManager() {
    isDoorOpen = false;
    doorOpenTime = 0;
}

void ServoManager::init() {
    // Inicjalizacja pinu w trybie LEDC: 50Hz (standard dla serw), 12-bit rozdzielczości
    // Dzięki temu serwo ma własny timer i nie "gryzie się" z wentylatorem 25kHz
    if (ledcAttach(PIN_SERVO_DOOR, 50, 12)) {
        Serial.println("Servo: Inicjalizacja LEDC (50Hz, 12-bit) udana.");
    }
    
    closeDoor(); // Ustawienie pozycji startowej
}

void ServoManager::setServoAngle(int angle) {
    // Ograniczamy kąt do bezpiecznego zakresu serwa SG90
    angle = constrain(angle, 0, 180);

    // Mapowanie kąta na wypełnienie (Duty Cycle) dla 12-bit (0-4095) przy 50Hz:
    // Standardowe serwo SG90 potrzebuje impulsu 0.5ms do 2.4ms (okres to 20ms)
    // 0 stopni -> 0.5ms/20ms * 4095 = ok. 102
    // 180 stopni -> 2.4ms/20ms * 4095 = ok. 491
    int duty = map(angle, 0, 180, 102, 491);
    
    ledcWrite(PIN_SERVO_DOOR, duty);
}

void ServoManager::openDoor() {
    if (!isDoorOpen) {
        setServoAngle(100); // Otwarcie drzwi pod kątem 100 stopni
        isDoorOpen = true;
        doorOpenTime = millis();
        Serial.println("Zamek: Drzwi OTWARTE (Sterowanie LEDC)");
    }
}

void ServoManager::closeDoor() {
    setServoAngle(0); // Powrót do 0 stopni
    isDoorOpen = false;
    Serial.println("Zamek: Drzwi ZAMKNIĘTE (Sterowanie LEDC)");
}

void ServoManager::loop() {
    // Mechanizm automatycznego zamykania po 5 sekundach
    if (isDoorOpen && (millis() - doorOpenTime >= doorOpenDuration)) {
        closeDoor();
    }
}

void ServoManager::processCommand(String jsonCommand) {
    // Alokacja dokumentu JSON (v7)
    JsonDocument doc; 

    // Próba deserializacji odebranej wiadomości MQTT
    DeserializationError error = deserializeJson(doc, jsonCommand);
    if (error) {
        Serial.print("Servo: Błąd JSON: ");
        Serial.println(error.c_str());
        return;
    }

    // Obsługa sterowania kątowego (np. z suwaka w aplikacji)
    if (doc.containsKey("angle")) {
        int angle = doc["angle"];
        setServoAngle(angle);
        
        // Wyłączamy automatyczne zamykanie, gdy użytkownik ręcznie ustawia kąt
        isDoorOpen = false; 
        Serial.printf("Servo: Ustawiono kąt %d przez MQTT\n", angle);
    }
    
    // Opcjonalna obsługa poleceń typu open/close
    if (doc.containsKey("state")) {
        String state = doc["state"];
        if (state == "OPEN") openDoor();
        else if (state == "CLOSE") closeDoor();
    }
}


// #include "ServoManager.h"

// ServoManager::ServoManager() {
//     isDoorOpen = false;
//     doorOpenTime = 0;
// }

// // void ServoManager::init() {
// //     // Biblioteka ESP32Servo wymaga przydzielenia sprzętowych timerów
// //     // Zostawiamy alokację wszystkich timerów - to bezpieczne dla ESP32
// //     //ESP32PWM::allocateTimer(0);
// //     // ESP32PWM::allocateTimer(1);
// //     // ESP32PWM::allocateTimer(2);
// //     // ESP32PWM::allocateTimer(3);

// //     // Standardowa częstotliwość dla serw SG90 to 50Hz
// //     // blindServo.setPeriodHertz(50); // Tymczasowo zakomentowane
// //     doorServo.setPeriodHertz(50);

// //     // Przypisanie pinów
// //     // blindServo.attach(PIN_SERVO_BLIND, 500, 2400); // Tymczasowo zakomentowane
// //     doorServo.attach(PIN_SERVO_DOOR, 500, 2400);

// //     // Ustawienie drzwi w pozycję startową (zamknięte)
// //     closeDoor();
// //     // setBlindPosition(0); // Tymczasowo zakomentowane
// // }

// void ServoManager::init() {
//     // Ustawiamy częstotliwość na 50Hz (standard dla serw)
//     // i rozdzielczość na 12 bitów (0-4095)
//     // analogWriteFrequency(PIN_SERVO_DOOR, 50);
//     // analogWriteResolution(PIN_SERVO_DOOR, 12);
    
//     ledcAttach(PIN_SERVO_DOOR, 50, 12);

//     closeDoor();
// }

// // void ServoManager::openDoor() {
// //     if (!isDoorOpen) {
// //         doorServo.write(100); // Obrót o 100 stopni
// //         isDoorOpen = 1;
// //         doorOpenTime = millis(); // Zapisujemy czas otwarcia
// //         Serial.println("Zamek: Drzwi otwarte!");
// //     }
// // }

// void ServoManager::openDoor() {
//     if (!isDoorOpen) {
//         // W 12-bitach (0-4095): 
//         // 0.5ms (0 stopni) -> 102
//         // 2.4ms (180 stopni) -> 491
//         // 100 stopni to około 320
//         analogWrite(PIN_SERVO_DOOR, 320); 
        
//         isDoorOpen = true;
//         doorOpenTime = millis();
//         Serial.println("Zamek: Drzwi otwarte (analogWrite)");
//     }
// }

// // void ServoManager::closeDoor() {
// //     doorServo.write(0); // Powrót na 0 stopni
// //     isDoorOpen = 0;
// //     Serial.println("Zamek: Drzwi zamkniete!");
// // }

// void ServoManager::closeDoor() {
//     // Powrót do 0 stopni (wartość ok. 102)
//     analogWrite(PIN_SERVO_DOOR, 102); 
//     isDoorOpen = false;
//     Serial.println("Zamek: Drzwi zamkniete (analogWrite)");
// }

// /* Tymczasowo zakomentowane (Żaluzja)
// void ServoManager::setBlindPosition(int angle) {
//     blindServo.write(angle);
// }
// */

// // void ServoManager::processCommand(String jsonCommand) {
// //     JsonDocument doc;
// //     if (deserializeJson(doc, jsonCommand)) return;

// //     if (doc.containsKey("angle")) {
// //         int angle = doc["angle"];
// //         angle = constrain(angle, 0, 180);
        
// //         // Mapujemy kąt 0-180 na wartości PWM 102-491
// //         int pwmValue = map(angle, 0, 180, 102, 491);
// //         analogWrite(PIN_SERVO_DOOR, pwmValue);
        
// //         Serial.printf("MQTT Test: Kat %d (PWM: %d)\n", angle, pwmValue);
// //         isDoorOpen = false; 
// //     }
// // }

// void ServoManager::loop() {
//     // Jeśli drzwi są otwarte i minęło 5 sekund -> zamknij je asynchronicznie
//     if (isDoorOpen && (millis() - doorOpenTime >= doorOpenDuration)) {
//         closeDoor();
//     }
// }

// // void ServoManager::processCommand(String jsonCommand) {
// //     JsonDocument doc;
// //     DeserializationError error = deserializeJson(doc, jsonCommand);
// //     if (error) return;

// //     if (doc.containsKey("angle")) {
// //         int angle = doc["angle"];
// //         // Zabezpieczenie przed uszkodzeniem trybów (serwo SG90 ma zakres 0-180)
// //         angle = constrain(angle, 0, 180); 
        
// //         doorServo.write(angle);
        
// //         Serial.print("Test MQTT: Serwo ustawione na kat: ");
// //         Serial.println(angle);
        
// //         // Wyłączamy flagę auto-zamykania, żeby serwo nie wróciło nagle po 5s podczas testów
// //         isDoorOpen = false; 
// //     }
// // }

// void ServoManager::processCommand(String jsonCommand) {
//     // 1. Deklaracja obiektu JSON (używamy JsonDocument dla ArduinoJson v7)
//     JsonDocument doc; 

//     // 2. Deserializacja - musisz to zrobić, zanim zaczniesz czytać z 'doc'
//     DeserializationError error = deserializeJson(doc, jsonCommand);
//     if (error) {
//         Serial.print("Błąd JSON: ");
//         Serial.println(error.c_str());
//         return;
//     }

//     // 3. Teraz 'doc' już istnieje i możesz z niego korzystać
//     if (doc.containsKey("angle")) {
//         int angle = doc["angle"];
//         angle = constrain(angle, 0, 180);
        
//         // Zamiast analogWrite, użyjemy ledc (rozwiązuje konflikt z wentylatorem)
//         setServoAngle(angle); 
        
//         Serial.printf("Serwo ustawione na kąt: %d\n", angle);
//     }
// }