#include "FanManager.h"

FanManager::FanManager() {
    isOn = false;
    currentSpeed = 50; // Domyślna prędkość 50%
}

void FanManager::init() {
    // Przypisujemy pin do sprzętowego generatora PWM (Nowe API ESP32 v3.x)
    ledcAttach(PIN_FAN_PWM, PWM_FREQ, PWM_RES);
    
    // analogWriteFrequency(PIN_FAN_PWM, 25000);
    // analogWriteResolution(PIN_FAN_PWM, 8);

    // Ustawiamy stan początkowy (wyłączony)
    updateFan();
}

// void FanManager::updateFan() {
//     if (isOn) {
//         // Jeśli włączamy: upewniamy się, że pin jest podpięty do PWM
//         ledcAttach(PIN_FAN_PWM, PWM_FREQ, PWM_RES);
        
//         uint32_t dutyCycle = map(currentSpeed, 0, 100, 0, 255);
//         ledcWrite(PIN_FAN_PWM, dutyCycle);
        
//         Serial.print("Wentylator ON: ");
//         Serial.println(currentSpeed);
//     } else {
//         // Jeśli wyłączamy: całkowicie odpinamy generator PWM od pinu
//         ledcDetach(PIN_FAN_PWM);
//         pinMode(PIN_FAN_PWM, OUTPUT);
//         digitalWrite(PIN_FAN_PWM, LOW); // Wymuszamy stan niski (0V)
        
//         Serial.println("Wentylator OFF - wymuszony stan LOW");
//     }
// }

// void FanManager::updateFan() {
//     if (isOn) {
//         uint32_t duty = map(currentSpeed, 0, 100, 0, 255);
//         analogWrite(PIN_FAN_PWM, duty);
//         Serial.printf("Wentylator ON: %d%%\n", currentSpeed);
//     } else {
//         analogWrite(PIN_FAN_PWM, 0); // Zatrzymanie
//         Serial.println("Wentylator OFF");
//     }
// }

void FanManager::updateFan() {
    uint32_t duty = isOn ? map(currentSpeed, 0, 100, 0, 255) : 0;
    ledcWrite(PIN_FAN_PWM, duty);
}

// void FanManager::processCommand(String jsonCommand) {
//     JsonDocument doc;
//     DeserializationError error = deserializeJson(doc, jsonCommand);
//     if (error) return;

//     if (doc.containsKey("state")) {
//         isOn = (doc["state"] == "ON");
//     }
    
//     if (doc.containsKey("speed")) {
//         currentSpeed = doc["speed"];
//         // Zabezpieczenie przed błędem z aplikacji (ograniczenie do 100%)
//         if (currentSpeed > 100) {
//             currentSpeed = 100;
//         }
//     }

//     // Uaktualnienie sygnału na podstawie nowych zmiennych
//     updateFan();
// }

void FanManager::processCommand(String jsonCommand) {
    JsonDocument doc;
    if (deserializeJson(doc, jsonCommand)) return;

    if (doc.containsKey("state")) isOn = (doc["state"] == "ON");
    if (doc.containsKey("speed")) currentSpeed = doc["speed"];
    
    updateFan();
}

int FanManager::getSpeed() {
    if (!isOn) {
        return 0; // Jeśli wentylator jest wyłączony, zwracamy 0%
    }
    return currentSpeed; // Zwracamy aktualną moc
}