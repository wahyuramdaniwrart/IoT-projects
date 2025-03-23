#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <MQUnifiedsensor.h>

// Definisi sensor dan parameter
#define PLACA "Arduino UNO"
#define TYPE "MQ-4"
#define RATIO_MQ4_CLEAN_AIR 4.4  // RS / R0 = 3.6 ppm  
#define FACTOR_ESCALA 0.1875F  // Skala untuk konversi ADC ke tegangan

// Objek untuk sensor dan ADC
MQUnifiedsensor MQ4(PLACA, TYPE);
Adafruit_ADS1115 ads1, ads2;

void setup() {
    Serial.begin(115200);
    delay(200);

    // Inisialisasi sensor MQ4
    MQ4.setRegressionMethod(1); // _PPM = a * ratio^b
    MQ4.init();

    // Inisialisasi ADS1115
    ads1.begin(0x48); // ADS1115 pertama (alamat 0x48)
    ads2.begin(0x49); // ADS1115 kedua (alamat 0x49)

    Serial.print("Calibrating please wait.");
    float calcR0 = 0;

    // Kalibrasi sensor dengan rata-rata 10 pembacaan
    for (int i = 0; i < 10; i++) {
        float voltios = readSensor(ads1, 3);  // Membaca sensor dari ADS1 channel A3
        MQ4.externalADCUpdate(voltios);
        calcR0 += MQ4.calibrate(RATIO_MQ4_CLEAN_AIR);
        Serial.print(".");
    }
    MQ4.setR0(calcR0 / 10);
    Serial.println(" done!");

    // Validasi nilai R0
    if (isinf(calcR0)) {
        Serial.println("Warning: R0 is infinite (Open circuit detected), check wiring!");
        while (1);
    }
    if (calcR0 == 0) {
        Serial.println("Warning: R0 is zero (Analog pin shorted to ground), check wiring!");
        while (1);
    }  
}

void loop() {
    // Membaca nilai dari 3 sensor
    float volt1 = readSensor(ads1, 3);  // Sensor 1 (ADS1 A3)
    float volt2 = readSensor(ads2, 0);  // Sensor 2 (ADS2 A0)
    float volt3 = readSensor(ads2, 1);  // Sensor 3 (ADS2 A1)

    MQ4.externalADCUpdate(volt1);
    float methane1 = calculateGas(1012.7, -2.786);
    
    MQ4.externalADCUpdate(volt2);
    float methane2 = calculateGas(1012.7, -2.786);
    
    MQ4.externalADCUpdate(volt3);
    float methane3 = calculateGas(1012.7, -2.786);

    Serial.print("| "); Serial.print(methane1);
    Serial.print(" | "); Serial.print(methane2);
    Serial.print(" | "); Serial.print(methane3);
    Serial.println(" |");
    
    delay(500);
}

// Fungsi untuk membaca tegangan dari ADS1115
float readSensor(Adafruit_ADS1115 &ads, uint8_t channel) {
    short adc = ads.readADC_SingleEnded(channel);
    return (adc * FACTOR_ESCALA) / 1000.0;
}

// Fungsi untuk menghitung konsentrasi gas berdasarkan persamaan regresi
float calculateGas(float a, float b) {
    MQ4.setA(a);
    MQ4.setB(b);
    return MQ4.readSensor();
}
