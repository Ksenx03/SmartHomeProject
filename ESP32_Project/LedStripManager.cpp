#include "LedStripManager.h"

// Konstruktor - tu inicjalizujemy nasze obiekty pasków
LedStripManager::LedStripManager() : 
    stripWew(NUM_LEDS_PER_STRIP, PIN_LED_WEW, NEO_GRB + NEO_KHZ800),
    stripZew(NUM_LEDS_PER_STRIP, PIN_LED_ZEW, NEO_GRB + NEO_KHZ800) {
}

void LedStripManager::init() {
    // Uruchamiamy komunikację z paskami
    stripWew.begin();
    stripWew.show(); // Inicjalizacja wyłączonego paska

    stripZew.begin();
    stripZew.show(); 
}

void LedStripManager::processCommand(String jsonCommand) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonCommand);
    if (error) return;

    String target = doc["target"]; 

    // ---- OBSŁUGA PASKA WEWNĘTRZNEGO ----
    if (target == "wew") {
        if (doc.containsKey("brightness")) {
            stripWew.setBrightness(doc["brightness"]);
        }
        
        if (doc.containsKey("state")) {
            if (doc["state"] == "ON" && doc.containsKey("color")) {
                // Włączenie na konkretny kolor (funkcja Color łączy R, G, B w jedną wartość)
                uint32_t color = stripWew.Color(doc["color"]["r"], doc["color"]["g"], doc["color"]["b"]);
                stripWew.fill(color);
            } else if (doc["state"] == "OFF") {
                // Wyłączenie = wypełnienie kolorem czarnym (0,0,0)
                stripWew.fill(stripWew.Color(0, 0, 0));
            }
        }
        stripWew.show(); // Aktualizujemy fizycznie TYLKO pasek wewnętrzny
    } 
    
    // ---- OBSŁUGA PASKA ZEWNĘTRZNEGO ----
    else if (target == "zew") {
        if (doc.containsKey("brightness")) {
            stripZew.setBrightness(doc["brightness"]);
        }
        
        if (doc.containsKey("state")) {
            if (doc["state"] == "ON" && doc.containsKey("color")) {
                uint32_t color = stripZew.Color(doc["color"]["r"], doc["color"]["g"], doc["color"]["b"]);
                stripZew.fill(color);
            } else if (doc["state"] == "OFF") {
                stripZew.fill(stripZew.Color(0, 0, 0));
            }
        }
        stripZew.show(); // Aktualizujemy fizycznie TYLKO pasek zewnętrzny
    }
}