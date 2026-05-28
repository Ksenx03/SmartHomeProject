#include <Wire.h>
#include <AHTxx.h>
#include <Adafruit_BMP280.h>

// Tworzymy obiekty z nowymi bibliotekami
AHTxx aht20(AHTXX_ADDRESS_X38, AHT2x_SENSOR); // Zdefiniowanie czujnika jako AHT20
Adafruit_BMP280 bmp;

bool ahtReady = false;
bool bmpReady = false;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("\n\n=============================================");
  Serial.println(" KOD DIAGNOSTYCZNY V2: AHTxx + BMP280");
  Serial.println("=============================================\n");
  
  Wire.begin(); 
  delay(1000); 

  // --- KROK 1: Inicjalizacja AHT20 (nowa biblioteka) ---
  Serial.println("--- Inicjalizacja AHT20 ---");
  if (aht20.begin()) {
    Serial.println("SUKCES: AHT20 (AHTxx) dziala poprawnie!");
    ahtReady = true;
  } else {
    Serial.println("BLAD: Czujnik AHT20 nadal nie odpowiada.");
  }

  // --- KROK 2: Inicjalizacja BMP280 ---
  Serial.println("\n--- Inicjalizacja BMP280 ---");
  if (bmp.begin(0x77)) {
    Serial.println("SUKCES: BMP280 dziala poprawnie!");
    bmpReady = true;
  } else {
    Serial.println("BLAD: BMP280 przestal dzialac.");
  }

  Serial.println("\nRozpoczynam pomiary...\n");
}

void loop() {
  Serial.println("-----------------------------------");
  
  if (ahtReady) {
    // Biblioteka AHTxx ma dedykowane funkcje odczytu
    float t = aht20.readTemperature();
    float h = aht20.readHumidity();

    if (t != AHTXX_ERROR) {
      Serial.print("[AHT20]  Temp:       "); 
      Serial.print(t); 
      Serial.println(" *C");
      
      Serial.print("[AHT20]  Wilgotnosc: "); 
      Serial.print(h); 
      Serial.println(" %");
    } else {
      Serial.println("[AHT20]  Odczyt zaklocony, ponawianie...");
    }
  }

  if (bmpReady) {
    float pressure = bmp.readPressure() / 100.0F; 
    Serial.print("[BMP280] Cisnienie:  "); 
    Serial.print(pressure); 
    Serial.println(" hPa");
  }

  delay(3000);
}