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

    if (target == "wew") {
    // 1. Сначала считываем режим, если он есть
    if (doc.containsKey("mode")) {
      currentWewEffectMode = doc["mode"].as<String>();
      currentWewEffectMode.toLowerCase();
    }

    if (doc.containsKey("brightness")) {
      stripWew.setBrightness(doc["brightness"]);
    }

    if (doc.containsKey("state")) {
      String stateStr = doc["state"].as<String>();
      stateStr.toUpperCase();

      if (stateStr == "ON") {
        wewIsOn = true;
        // КЛЮЧЕВОЕ ИЗМЕНЕНИЕ: 
        // Если мы НЕ в режиме спецэффекта, тогда применяем статический цвет
        if (currentWewEffectMode == "none" && doc.containsKey("color")) {
          uint32_t color = stripWew.Color(doc["color"]["r"], doc["color"]["g"], doc["color"]["b"]);
          stripWew.fill(color);
        }
      } else {
        wewIsOn = false;
        currentWewEffectMode = "none";
        stripWew.fill(stripWew.Color(0, 0, 0));
      }
    }
    stripWew.show();
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
    if (currentAlarmMode != NONE) {
        float pulse = (sin(millis() / 200.0) * 102.5) + 152.5;
        uint8_t br = (uint8_t)pulse;
        uint32_t color;
        
        if (currentAlarmMode == GAS_ALARM) {
            color = stripWew.Color(br, br, 0); // ЖЕЛТЫЙ (R+G)
        } else if (currentAlarmMode == WATER_ALARM) {
            color = stripWew.Color(0, 0, br);  // СИНИЙ (B)
        } else if (currentAlarmMode == INTRUSION_ALARM) {
            color = stripWew.Color(br, 0, 0);  // КРАСНЫЙ (R)
        }
        
        stripWew.fill(color);
        stripZew.fill(color);
        stripWew.show();
        stripZew.show();
        return;
    }

    // 2. ГЛАВНАЯ ПРОВЕРКА
    // Если свет выключен или режим "none", мы ничего не увидим
    if (!wewIsOn) return; 
    if (currentWewEffectMode == "none") return;

    unsigned long now = millis();

    // 3. ЭФФЕКТЫ
    if (currentWewEffectMode == "disco") {
        if (now - lastWewEffectTime > 200) {
            Serial.println("RUNNING: Disco Effect"); // Маячок!
            uint32_t discoColor = stripWew.Color(random(255), random(255), random(255));
            stripWew.fill(discoColor);
            stripWew.show();
            lastWewEffectTime = now;
        }
    } 
    else if (currentWewEffectMode == "strobe") {
        if (now - lastWewEffectTime > 80) {
            Serial.println("RUNNING: Strobe Effect"); // Маячок!
            wewStrobeState = !wewStrobeState;
            stripWew.fill(wewStrobeState ? stripWew.Color(255, 255, 255) : 0);
            stripWew.show();
            lastWewEffectTime = now;
        }
    } 
    else if (currentWewEffectMode == "relax") {
        // Тут мы не принтуем постоянно, чтобы не спамить в консоль
        float breath = (sin(now / 1000.0) * 80) + 100;
        stripWew.fill(stripWew.Color((uint8_t)breath, (uint8_t)(breath * 0.3), 0));
        stripWew.show();
    }
}