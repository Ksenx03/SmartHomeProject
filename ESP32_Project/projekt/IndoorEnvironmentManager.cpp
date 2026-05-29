#include "IndoorEnvironmentManager.h"

// KONSTRUKTOR: Podajemy zdefiniowane parametry DOKŁADNIE jak w dzialajacym kodzie V2!
IndoorEnvironmentManager::IndoorEnvironmentManager() : aht20(AHTXX_ADDRESS_X38, AHT2x_SENSOR) {
    lastReadTime = 0;
    currentTemp = 0.0;
    currentHum = 0.0;
    currentPress = 0.0;
    
    isInitialized = false;
    ahtReady = false;
    bmpReady = false;
}

void IndoorEnvironmentManager::init() {
    Serial.println("\n--- INICJALIZACJA MODULU AHT20 + BMP280 ---");
    
    Wire.begin(); 
    delay(1000); // 1000ms opoznienia - dokladnie tak jak w dzialajacym skrypcie!
    
    // --- KROK 1: Inicjalizacja AHT20 ---
    ahtReady = aht20.begin();
    if (!ahtReady) {
        Serial.println("[AHT20] BLAD: Czujnik AHT20 nadal nie odpowiada.");
    } else {
        Serial.println("[AHT20] SUKCES: AHT20 (AHTxx) dziala poprawnie!");
    }

    // --- KROK 2: Inicjalizacja BMP280 ---
    bmpReady = bmp.begin(0x77); 
    if (!bmpReady) {
        Serial.println("[BMP280] BLAD: BMP280 przestal dzialac.");
    } else {
        Serial.println("[BMP280] SUKCES: BMP280 dziala poprawnie!");
    }

    // Uruchom menedżera, jeśli działa CHOCIAŻ JEDEN podzespół
    if (ahtReady || bmpReady) {
        isInitialized = true;
        Serial.println("[AHT20+BMP280] STATUS: Menedzer mikroklimatu gotowy.");
    } else {
        isInitialized = false;
        Serial.println("[AHT20+BMP280] STATUS: KRYTYCZNA AWARIA OBU UKLADOW.");
    }
    Serial.println("---------------------------------------------------------\n");
}

bool IndoorEnvironmentManager::loop() {
    if (!isInitialized) return false;

    if (millis() - lastReadTime >= readInterval) {
        lastReadTime = millis();

        // --- POBIERANIE DANYCH Z AHT20 ---
        if (ahtReady) {
            float t = aht20.readTemperature();
            float h = aht20.readHumidity();

            if (t != AHTXX_ERROR) {
                currentTemp = t;
                currentHum = h;
            }
        }
        
        // --- POBIERANIE DANYCH Z BMP280 ---
        if (bmpReady) {
            currentPress = bmp.readPressure() / 100.0F; // Pa -> hPa
        }
        
        return true; 
    }
    return false;
}

String IndoorEnvironmentManager::getSensorJson() {
    JsonDocument doc;
    
    // Zamykamy dane w obiekcie "indoor"
    JsonObject indoor = doc["indoor"].to<JsonObject>();
    indoor["temperature"] = currentTemp;
    indoor["humidity"] = currentHum;
    indoor["pressure"] = currentPress;
    
    String output;
    serializeJson(doc, output);
    return output;
}

float IndoorEnvironmentManager::getTemp() { return currentTemp; }
float IndoorEnvironmentManager::getHum() { return currentHum; }
float IndoorEnvironmentManager::getPress() { return currentPress; }