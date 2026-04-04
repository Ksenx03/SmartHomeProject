#include "LedStripManager.h"

LedStripManager::LedStripManager()
  : stripWew(NUM_LEDS_PER_STRIP, PIN_LED_WEW, NEO_GRB + NEO_KHZ800),
    stripZew(NUM_LEDS_PER_STRIP, PIN_LED_ZEW, NEO_GRB + NEO_KHZ800) {
}

void LedStripManager::init() {
  // Uruchamiamy komunikację z paskami
  stripWew.begin();
  stripWew.show();  // Inicjalizacja wyłączonego paska

  stripZew.begin();
  stripZew.show();

  wewIsOn = false;
  zewIsOn = false;
}

// void LedStripManager::processCommand(String jsonCommand) {
//   JsonDocument doc;
//   DeserializationError error = deserializeJson(doc, jsonCommand);
//   if (error) return;

//   String target = doc["target"];

//   // ---- OBSŁUGA PASKA WEWNĘTRZNEGO ----
//   if (target == "wew") {
//     if (doc.containsKey("brightness")) {
//       stripWew.setBrightness(doc["brightness"]);
//     }

//     if (doc.containsKey("state")) {
//       if (doc["state"] == "ON" && doc.containsKey("color")) {
//         // Włączenie na konkretny kolor (funkcja Color łączy R, G, B w jedną wartość)
//         uint32_t color = stripWew.Color(doc["color"]["r"], doc["color"]["g"], doc["color"]["b"]);
//         stripWew.fill(color);
//       } else if (doc["state"] == "OFF") {
//         // Wyłączenie = wypełnienie kolorem czarnym (0,0,0)
//         stripWew.fill(stripWew.Color(0, 0, 0));
//       }
//     }
//     stripWew.show();  // Aktualizujemy fizycznie TYLKO pasek wewnętrzny
//   }

//   // ---- OBSŁUGA PASKA ZEWNĘTRZNEGO ----
//   else if (target == "zew") {
//     if (doc.containsKey("brightness")) {
//       stripZew.setBrightness(doc["brightness"]);
//     }

//     if (doc.containsKey("state")) {
//       if (doc["state"] == "ON" && doc.containsKey("color")) {
//         uint32_t color = stripZew.Color(doc["color"]["r"], doc["color"]["g"], doc["color"]["b"]);
//         stripZew.fill(color);
//       } else if (doc["state"] == "OFF") {
//         stripZew.fill(stripZew.Color(0, 0, 0));
//       }
//     }
//     stripZew.show();  // Aktualizujemy fizycznie TYLKO pasek zewnętrzny
//   }
// }

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
      String stateStr = doc["state"].as<String>();
      stateStr.toUpperCase();  // Zabezpieczenie przed małymi literami

      if (stateStr == "ON") {
        wewIsOn = true;  // <--- TEGO BRAKOWAŁO! Zapisujemy stan w pamięci obiektu

        if (doc.containsKey("color")) {
          // Jeśli podano kolor, zapalamy na konkretny
          uint32_t color = stripWew.Color(doc["color"]["r"], doc["color"]["g"], doc["color"]["b"]);
          stripWew.fill(color);
        } else {
          // Jeśli tylko włączono, ale nie podano koloru - dajemy domyślny biały
          stripWew.fill(stripWew.Color(255, 255, 255));
        }
      } else if (stateStr == "OFF") {
        wewIsOn = false;  // <--- TEGO BRAKOWAŁO!
        stripWew.fill(stripWew.Color(0, 0, 0));
      }
    }
    stripWew.show();  // Aktualizujemy fizycznie TYLKO pasek wewnętrzny
  }

  // ---- OBSŁUGA PASKA ZEWNĘTRZNEGO ----
  else if (target == "zew") {
    if (doc.containsKey("brightness")) {
      stripZew.setBrightness(doc["brightness"]);
    }

    if (doc.containsKey("state")) {
      String stateStr = doc["state"].as<String>();
      stateStr.toUpperCase();

      if (stateStr == "ON") {
        zewIsOn = true;  // <--- Zapisujemy stan!

        if (doc.containsKey("color")) {
          uint32_t color = stripZew.Color(doc["color"]["r"], doc["color"]["g"], doc["color"]["b"]);
          stripZew.fill(color);
        } else {
          stripZew.fill(stripZew.Color(255, 255, 255));
        }
      } else if (stateStr == "OFF") {
        zewIsOn = false;  // <--- Zapisujemy stan!
        stripZew.fill(stripZew.Color(0, 0, 0));
      }
    }
    stripZew.show();
  }
}

String LedStripManager::getWewState() {
  // Jeśli używasz booleana, np. wewIsOn:
  return wewIsOn ? "ON" : "OFF";
}

String LedStripManager::getZewState() {
  // Jeśli używasz booleana, np. zewIsOn:
  return zewIsOn ? "ON" : "OFF";
}

void LedStripManager::setAlarmMode(AlarmMode mode) {
  currentAlarmMode = mode;
}

void LedStripManager::update() {
  if (currentAlarmMode == NONE) return;

  // Obliczanie jasności (sinusoida: od 50 do 255)
  float pulse = (sin(millis() / 200.0) * 102.5) + 152.5;
  uint8_t brightness = (uint8_t)pulse;

  uint32_t color;
  if (currentAlarmMode == GAS_ALARM) {
    color = stripWew.Color(0, brightness, 0);  // Zielony gaz
  } else if (currentAlarmMode == WATER_ALARM) {
    color = stripWew.Color(0, 0, brightness);  // Niebieski woda
  } else if (currentAlarmMode == INTRUSION_ALARM) {
    // Szybkie, agresywne pulsowanie na włamanie (zwiększony mianownik z 500 do 150)
    float fastPulse = (sin(millis() / 150.0) * 127.5) + 127.5;
    color = stripWew.Color((uint8_t)fastPulse, 0, 0);  // Czerwony ruch
  }

  stripWew.fill(color);
  stripZew.fill(color);
  stripWew.show();
  stripZew.show();
}