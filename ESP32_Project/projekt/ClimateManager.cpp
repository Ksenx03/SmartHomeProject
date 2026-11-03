#include "ClimateManager.h"

// Konstruktor przypisujący pin 16 do zmiennej relayPin
ClimateManager::ClimateManager(int pin) {
    relayPin = pin;
    isActive = false;
    targetTemp = 20.0;
}

void ClimateManager::init() {
    pinMode(relayPin, OUTPUT);
    // Wymuszamy stan niski na starcie systemu, aby Peltier nie grzał od razu
    digitalWrite(relayPin, LOW); 
}

// Funkcja włączająca lub wyłączająca grzanie
void ClimateManager::setActive(bool state) {
    isActive = state;
    
    // Jeśli state to true (ON), wysyłamy HIGH na przekaźnik. W przeciwnym razie LOW.
    if (isActive) {
        digitalWrite(relayPin, HIGH); 
        Serial.println("Ogrzewanie ON - przekaznik zwarty");
    } else {
        digitalWrite(relayPin, LOW);
        Serial.println("Ogrzewanie OFF - przekaznik rozwarty");
    }
}

bool ClimateManager::getIsActive() {
    return isActive;
}

void ClimateManager::setTargetTemp(float temp) {
    targetTemp = temp;
}

// Prosty przykład funkcji, która reguluje wentylator na podstawie grzania
int ClimateManager::update(float currentTemp) {
    if (isActive) {
        // Kiedy grzejemy Peltierem, wentylator musi kręcić się na 100%, aby rozprowadzać ciepło
        return 100; 
    }
    
    // Kiedy nie grzejemy, możemy wyłączyć wentylator (lub ustawić inną wartość)
    return 0; 
}