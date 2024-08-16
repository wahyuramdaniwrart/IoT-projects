#include <EmonLib.h>
#include "SPI.h"
#include "TFT_22_ILI9225.h"
#include <SD.h>
#include <../fonts/FreeSans12pt7b.h>
#include <../fonts/FreeSans24pt7b.h>


#define TFT_RST 48
#define TFT_RS 49
#define TFT_CS 7
#define TFT_SDI 51
#define TFT_CLK 52
#define TFT_LED 0
#define TFT_BRIGHTNESS 200 
#define SD_CS 35

TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_LED, TFT_BRIGHTNESS);

int sct_pin = A14;
int vol_pin = A0;

double Irms, prevIrms = -1;
float Vrms, prevVrms = -1;


EnergyMonitor emon1;

const double currentOffset = 16.05;

bool sdCardPresent = true;

void setup() {
  Serial.begin(9600);   
  tft.begin();
  tft.clear();
  tft.setOrientation(1); 
  tft.setBackgroundColor(COLOR_BLACK);

  emon1.current(sct_pin, 108.225);
  analogReference(5.0);

  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("Card failed, or not present");
    sdCardPresent = false;
  } else {
    Serial.println("Card initialized.");
    sdCardPresent = true;

    if (!SD.exists("datalog.CSV")) {
      File dataFile = SD.open("datalog.CSV", FILE_WRITE);
      if (dataFile) {
        dataFile.println("Current, Voltage");
        dataFile.close();
        Serial.println("datalog.CSV created.");
      } else {
        Serial.println("Error creating datalog.CSV");
      }
    } else {
      Serial.println("datalog.CSV already exists.");
    }
  }
} 

void loop() {
  if (sdCardPresent && !SD.begin(SD_CS)) {
    // SD card was removed
    sdCardPresent = false;
    tft.clear();
    tft.setFont(Terminal12x16);
    tft.drawText(0, 50, "SD Card Removed!", COLOR_RED);
    Serial.println("SD Card Removed! Stopping sensor reading.");
    return;
  }

  unsigned long currentMillis = millis();
  static unsigned long lastUpdateTime = 0;
  static unsigned long lastSaveTime = 0;
  
  if (currentMillis - lastUpdateTime >= 100) { 
    bool shouldUpdate = false;
    
    double newIrms = emon1.calcIrms(1500);
    if (newIrms < 0) newIrms = 0; 
    if (newIrms != prevIrms) {
      prevIrms = newIrms;
      shouldUpdate = true;
    }
    
    float newVrms = readVoltage();
    if (newVrms != prevVrms) {
      prevVrms = newVrms;
      shouldUpdate = true;
    }
    
    if (shouldUpdate) {
      tft.clear();
      displayValues(newIrms, newVrms);
    }
    
    if (currentMillis - lastSaveTime >= 30000) {
      saveData(newIrms, newVrms);
      lastSaveTime = currentMillis;
    }
    
    lastUpdateTime = currentMillis;
  }
}

void displayValues(double current, float voltage) {
  tft.setFont(Terminal12x16);
  tft.drawText(65, 0, "Current: ", COLOR_WHITE);
  tft.setFont(Trebuchet_MS16x21);
  tft.drawText(80, 30, String(current), COLOR_WHITE);

  tft.drawLine(0, 75, tft.maxX() - 1, 75, COLOR_WHITE);
  
  tft.setFont(Terminal12x16);
  tft.drawText(65, 90, "Voltage: ", COLOR_WHITE);
  tft.setFont(Trebuchet_MS16x21);
  tft.drawText(60, 120, String(voltage), COLOR_WHITE);
}

float readVoltage() {
  // Mendapatkan nilai amplitudo (nilai puncak)
  uint32_t adc_value = get_max();
  
  // Menghitung tegangan aktual (dengan referensi tegangan ADC = 5.0V)
  float voltage = (adc_value * 5000 / 1023.0) / sqrt(2.0);
  
  // Mengurangi offset konstan
  float voltage_calb = voltage - 10.02;
  
  // Jika nilai tegangan negatif, atur menjadi 0
  if (voltage_calb < 0) {
    voltage_calb = 0;
  }

  Serial.println(voltage);
  return voltage_calb ;
}

uint16_t get_max() {
  uint16_t max_v = 0;
  for(uint8_t i = 0; i < 200; i++) {
    uint16_t r = analogRead(vol_pin);  
    if(max_v < r) max_v = r;
    delayMicroseconds(100);
  }
  return max_v;
}

void saveData(double current, float voltage) {
  if (!sdCardPresent) return;

  String dataString = String(current) + ", " + String(voltage);
  
  File dataFile = SD.open("datalog.CSV", FILE_WRITE);

  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    Serial.println("Data saved: " + dataString);
  } else {
    Serial.println("Error opening datalog.CSV");
  }
}
