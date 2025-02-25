#include "mpx5700ap.h"
#include <Ewma.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>
#include "MQUnifiedsensor.h"

// Konfigurasi ADS1115
Adafruit_ADS1115 ads1;
Adafruit_ADS1115 ads2;

float factorEscala = 0.1875F;

// Konfigurasi DS18B20
#define ONE_WIRE_BUS 26
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
float suhu[3];

// Konfigurasi MPX5700AP
#define TC_PIN1 32
#define TC_PIN2 34
#define TC_PIN3 35
#define DIVIDER_RATIO 1.45

mpx5700 mpx;
Ewma adcFilter1(0.1);
Ewma adcFilter2(0.1);
Ewma adcFilter3(0.1);
float tekanan1, tekanan2, tekanan3;


//DEFINISI MQ SENSOR
float MQ4_1, MQ4_2, MQ4_3, MQ135_1, MQ135_2, MQ135_3;

// Konfigurasi DHT11
#define DHT_PIN 14
DHT dht(DHT_PIN, DHT11);
float suhu_dht, kelembaban;

// Konfigurasi OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Konfigurasi RTC
RTC_DS3231 rtc;

// Konfigurasi SD Card
#define SD_CS 5
String data_save;


float readMPX(int pin, Ewma &filter) {
    int raw = analogRead(pin) - 520;
    float filtered = filter.filter(raw);
    float V_adc = (filtered * 3.3) / 4095;
    float V_sensor = ((V_adc / (5.0 * DIVIDER_RATIO))) / 0.0012858;

    // Terapkan regresi linear y = 42.5x - 13
     float calibrated_pressure = (2.3488 * V_sensor) - 0.6846;
    
    // Pastikan nilai tidak negatif
    return calibrated_pressure < 0 ? 0 : calibrated_pressure;
}

float mq135Calb(float CO2_1_VOLT) {
    float calibrasi = (3601.9 * CO2_1_VOLT * CO2_1_VOLT) - (1190.6 * CO2_1_VOLT) + 461.63; 
    return calibrasi;
}

float mq4Calb(float CH4_1_VOLT) {
  float calibrasi = (0.0021 * CH4_1_VOLT * CH4_1_VOLT) + (1.9768 * CH4_1_VOLT) + 11.743; 
  return  calibrasi;
}

void setup() {
    Serial.begin(115200);
    Wire.begin();
    ads1.begin(0x48);
    ads2.begin(0x49);
    
    ds18b20.begin();
    dht.begin();
    
    if (!rtc.begin()) Serial.println("RTC Tidak Ditemukan!");
    if (!SD.begin(SD_CS)) Serial.println("SD Card initialization failed!");
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) Serial.println("SSD1306 initialization failed!");
    display.clearDisplay();
    display.display();
}

void loop() {
    readSensors();
    logData();
    displayData();
    delay(1000);
}

void readSensors() {
    //suhu bahan
    ds18b20.requestTemperatures();
    for (int i = 0; i < 3; i++) suhu[i] = ds18b20.getTempCByIndex(i);
    //tekanan
    tekanan1 = readMPX(TC_PIN1, adcFilter1);
    tekanan2 = readMPX(TC_PIN2, adcFilter2);
    tekanan3 = readMPX(TC_PIN3, adcFilter3);
    //dht11
    suhu_dht = dht.readTemperature();
    kelembaban = dht.readHumidity();
    //mq135
    float CO2_1_VOLT = readVoltage(ads1, 0);
    float CO2_2_VOLT = readVoltage(ads1, 1); //+ 0.35;
    float CO2_3_VOLT = readVoltage(ads1, 2); // + 0.19;
    MQ135_1 = mq135Calb(CO2_1_VOLT); 
    MQ135_2 = mq135Calb(CO2_2_VOLT);
    MQ135_3 = mq135Calb(CO2_3_VOLT);
    //mq4
    float CH4_1_VOLT = ads1.readADC_SingleEnded(3);
    float CH4_2_VOLT = ads2.readADC_SingleEnded(0);
    float CH4_3_VOLT = ads2.readADC_SingleEnded(1);
    MQ4_1 = mq4Calb(CH4_1_VOLT); 
    MQ4_2 = mq4Calb(CH4_2_VOLT);
    MQ4_3 = mq4Calb(CH4_3_VOLT);
}

float readVoltage(Adafruit_ADS1115 &ads, int channel) {
    float adc = ads.readADC_SingleEnded(channel);
    float voltage = ((adc * factorEscala) / 1000.0);
    return voltage;
}


void logData() {
    DateTime now = rtc.now();
    data_save = String(tekanan1) + "," + String(tekanan2) + "," + String(tekanan3) + "," +
                String(suhu[0]) + "," + String(suhu[1]) + "," + String(suhu[2]) + "," +
                String(MQ135_1) + "," + String(MQ135_2) + "," + String(MQ135_3) + "," +
                String(MQ4_1) + "," + String(MQ4_2) + "," + String(MQ4_3) + "," +
                String(suhu_dht) + "," + String(kelembaban) + "," +
                String(now.timestamp()) + "\n";
    appendFile(SD, "/monitoring.csv", data_save.c_str());
}

void displayData() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("MQ135:");
    display.println(String(MQ135_1) + "|" + String(MQ135_2) + "|" + String(MQ135_3));
    display.println("MQ4:");
    display.println(String(MQ4_1) + " | " + String(MQ4_2) + " | " + String(MQ4_3));
    display.println("Tekanan:");
    // DIKOREKSI: Tambah tanda kurung penutup
    // display.println(String(tekanan1) + " | " + String(tekanan2) + " | " + String(tekanan3));
    // display.println("Suhu:");
    // display.println(String(suhu[0]) + " | " + String(suhu[1]) + " | " + String(suhu[2]));
    // display.println("DHT:");
    // display.println("Suhu:" + String(suhu_dht) + "C");
    // display.println("Kelembaban:" + String(kelembaban) + "%");
    display.display();
}

void appendFile(fs::FS &fs, const char * path, const char * message) {
    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return;
    }
    file.print(message);
    file.close();
}
