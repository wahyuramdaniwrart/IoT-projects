#include <QuickStats.h>
#include <RTClib.h>
#include <SD.h>
#include <Adafruit_ADS1X15.h>

//definisi library

Adafruit_ADS1115 ads;
const float R1 = 1000000.0, R2 = 10000.0;
const float voltageMultiplier = (R1 + R2) / R2;
const float rShunt = 0.25;
const float multiplier = 0.0078125;

QuickStats statsDc; //initialize an instance of this class
QuickStats statsADc; //initialize an instance of this class
#define NUM_READINGSDc 10  // Jumlah sampel untuk statistik
#define NUM_READINGSADc 40  // Jumlah sampel untuk statistik
float adcReadingsDc[NUM_READINGSDc];  // Array untuk menyimpan hasil ADC
float adcReadingsADc[NUM_READINGSADc];  // Array untuk menyimpan hasil ADC




void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  ads.begin(0x48);
  ads.setGain(GAIN_SIXTEEN);
  

}

void loop() {
  // put your main code here, to run repeatedly:
  float Volt = tegangan();
  float Arus = arus();

  Serial.print("tegangan : ");
  Serial.println(Volt);
  Serial.print("arus : ");
  Serial.println(Arus);

  delay(1000);

}

float tegangan() {
  // Mengisi array dengan hasil pembacaan ADC
  for (int i = 0; i < NUM_READINGSDc; i++) {
    adcReadingsDc[i] = ads.readADC_SingleEnded(2) ;
  }

  // Mencari nilai maksimum dari pembDcaan
  float voltageCalb = statsDc.maximum(adcReadingsDc, NUM_READINGSDc);
  voltageCalb = ads.computeVolts(voltageCalb) * voltageMultiplier;

  if(voltageCalb < 0.00) {
    voltageCalb = 0.00;
  }
  
  return voltageCalb;
}

float arus() {
  // Mengisi array dengan hasil pembacaan ADC
  for (int i = 0; i < NUM_READINGSADc; i++) {
    adcReadingsADc[i] = ads.readADC_Differential_0_1();
  }

  float voltageShunt = statsADc.maximum(adcReadingsADc, NUM_READINGSADc);
  float vShunt = voltageShunt * multiplier;
  vShunt = vShunt / rShunt;

  return vShunt;

}
