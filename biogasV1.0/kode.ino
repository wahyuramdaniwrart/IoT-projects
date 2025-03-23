#include <MQUnifiedsensor.h>
#include <Ewma.h>
#include <EwmaT.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_ADS1X15.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <SPI.h>
#include <SD.h>
#include "gambar.h"

#define WIFI_SSID "Bqozhr"
#define WIFI_PASSWORD "Aryakmprt79"
#define MQTT_BROKER "mqtt.eclipseprojects.io"
#define MQTT_PORT 1883

WiFiClient espClient;
PubSubClient client(espClient);

// Konfigurasi ADS1115
Adafruit_ADS1115 ads1;
Adafruit_ADS1115 ads2;

// Konfigurasi DS18B20
#define ONE_WIRE_BUS 33
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
float suhu[3];

// Konfigurasi MPX5700AP

#define TC_PIN1 32
#define TC_PIN2 34
#define TC_PIN3 35
#define AREF 3.3           // set to AREF, typically board voltage like 3.3 or 5.0
#define ADC_RESOLUTION 12  // set to ADC bit resolution, 10 is default
#define DIVIDER_RATIO 1.45

Ewma adcFilter1(0.2);
Ewma adcFilter2(0.2);
Ewma adcFilter3(0.2);
float tekanan1, tekanan2, tekanan3;

 float get_voltage(int raw_adc) {
    return raw_adc * (AREF / (pow(2, ADC_RESOLUTION) - 1));
  }

//mq sensor
#define PLACA "ESP32"
#define TYPE "MQ-4"
#define RATIO_MQ4_CLEAN_AIR 4.4
#define FACTOR_ESCALA 0.125F
#define A_METHANE 2000.0
#define B_METHANE -2.90
#define OFFSET_SAMPLES 20

MQUnifiedsensor MQ41(PLACA, TYPE);
MQUnifiedsensor MQ42(PLACA, TYPE);
MQUnifiedsensor MQ43(PLACA, TYPE);

float baselineOffset[3] = {0, 0, 0};
float MQ4_1, MQ4_2, MQ4_3, MQ135_1, MQ135_2, MQ135_3;

// Konfigurasi DHT11
#define DHT_PIN 14
DHT dht(DHT_PIN, DHT11);
float suhu_dht, kelembaban;

// Konfigurasi OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Variabel Status
bool wifiStatus = false;
bool rekamData = false;
bool kirimData = false;

// Konfigurasi SD Card
#define SD_CS 5

// Timer
unsigned long previousMillis = 0;
const long interval = 5000; // Interval pembaruan tampilan (5 detik)
int sensorIndex = 0;



void setup() {
    Serial.begin(115200);

    // Inisialisasi WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Connecting to WiFi...");
    wifiStatus = (WiFi.status() == WL_CONNECTED);

    // Inisialisasi MQTT
    client.setServer(MQTT_BROKER, MQTT_PORT);

    // Inisialisasi Sensor
    ads1.begin(0x48);
    ads2.begin(0x49);
    ads1.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
    ads2.setGain(GAIN_ONE); 
    ds18b20.begin();
    dht.begin();

    // Inisialisasi SD Card
    if (!SD.begin(SD_CS)) {
        Serial.println("SD Card initialization failed!");
        rekamData = false;
    } else {
        rekamData = true;
    }

    // Inisialisasi OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 initialization failed!");
    }
    display.clearDisplay();
    display.drawBitmap(0, 0, wrart, 128, 64, WHITE);
    display.display();

    //inisialisasi mq4
    MQ41.setRegressionMethod(1);
    MQ42.setRegressionMethod(1);
    MQ43.setRegressionMethod(1);
    MQ41.init();
    MQ42.init();
    MQ43.init();
    Serial.print("Calibrating...");
    calibrateSensor(MQ41, ads1, 3, 0);
    calibrateSensor(MQ42, ads2, 0, 1);
    calibrateSensor(MQ43, ads2, 1, 2);
    Serial.println(" done!");
}

void loop() {
    unsigned long currentMillis = millis();

    // Periksa status WiFi secara berkala
    wifiStatus = (WiFi.status() == WL_CONNECTED);

    // Periksa status SD Card secara berkala
    rekamData = SD.begin(SD_CS);

    // Periksa status MQTT secara berkala
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    // Pembacaan sensor setiap 5 detik
    if (currentMillis - previousMillis >= 1000) {
        previousMillis = currentMillis;

        // Baca semua sensor
        readSensors();

        // Tampilkan data di LCD
        displayData();

        // Kirim data ke MQTT
        mqtt();

        // Ganti tampilan sensor
        sensorIndex = (sensorIndex + 1) % 5;
    }

    if (currentMillis - previousMillis >= 3600000) {
        // Rekam data ke SD Card
        logData();
        previousMillis = currentMillis;

    }
}

void readSensors() {
    // Baca suhu DS18B20
    ds18b20.requestTemperatures();
    for (int i = 0; i < 3; i++) {
        suhu[i] = ds18b20.getTempCByIndex(i);
    }

    // Baca tekanan MPX5700AP
    tekanan1 = readMPX(TC_PIN1, adcFilter1) - 5.63;
    tekanan2 = readMPX(TC_PIN2, adcFilter2) - 5.63;
    tekanan3 = readMPX(TC_PIN3, adcFilter3) - 6.63;

    // Baca DHT11
    suhu_dht = dht.readTemperature();
    kelembaban = dht.readHumidity();

    // Baca MQ135 dan MQ4
    MQ135_1 = mq135Calb(readVoltage(ads1, 0));
    MQ135_2 = mq135Calb(readVoltage(ads1, 1));
    MQ135_3 = mq135Calb(readVoltage(ads1, 2));
    
    float methane[] = {
        calculateGas(MQ41, readVoltage(ads1, 3)) - baselineOffset[0],
        calculateGas(MQ42, readVoltage(ads2, 0)) - baselineOffset[1],
        calculateGas(MQ43, readVoltage(ads2, 1)) - baselineOffset[2]
    };
    
    for (int i = 0; i < 3; i++) {
        if (methane[i] < 0) methane[i] = 0; // Hindari nilai negatif
        if (methane[i] > 100000) methane[i] = 100000; // Hindari nilai negatif
    }

    MQ4_1 = methane[0];
    MQ4_2 = methane[1];
    MQ4_3 = methane[2];

}

void displayData() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 7);
    display.println("BIO GAS");

    // Tampilkan status perangkat
    display.drawBitmap(70, 0, wifiStatus ? wifi : nowifi, 18, 18, WHITE);
    display.drawBitmap(90, 0, rekamData ? sdcard : nosdcard, 18, 18, WHITE);
    display.drawBitmap(110, 0, kirimData ? pesan : nopesan, 18, 18, WHITE);

    // Garis pemisah
    display.drawLine(0, 20, 128, 20, SSD1306_WHITE);

    // Tampilkan data sensor
    display.setTextSize(1);
    display.setCursor(0, 25);
    switch (sensorIndex) {
        case 0:
            display.println("MQ135 (ppm):");
            display.printf("1: %.2f\n", MQ135_1);
            display.printf("2: %.2f\n", MQ135_2);
            display.printf("3: %.2f\n", MQ135_3);
            break;
        case 1:
            display.println("MQ4 (ppm):");
            display.printf("1: %.2f\n", MQ4_1);
            display.printf("2: %.2f\n", MQ4_2);
            display.printf("3: %.2f\n", MQ4_3);
            break;
        case 2:
            display.println("Tekanan (kPa):");
            display.printf("1: %.2f\n", tekanan1);
            display.printf("2: %.2f\n", tekanan2);
            display.printf("3: %.2f\n", tekanan3);
            break;
        case 3:
            display.println("Suhu Bahan (C):");
            display.printf("1: %.2f\n", suhu[0]);
            display.printf("2: %.2f\n", suhu[1]);
            display.printf("3: %.2f\n", suhu[2]);
            break;
        case 4:
            display.println("DHT:");
            display.printf("Suhu: %.2f C\n", suhu_dht);
            display.printf("Kelembaban: %.2f%%\n", kelembaban);
            break;
    }

    display.display();
}

void mqtt() {
    if (!client.connected()) {
        reconnect();
    }

    char payload[20];
    snprintf(payload, sizeof(payload), "%.2f", MQ135_1);
    client.publish("sensor/mq135_1", payload);

    snprintf(payload, sizeof(payload), "%.2f", MQ135_2);
    client.publish("sensor/mq135_2", payload);
    
    snprintf(payload, sizeof(payload), "%.2f", MQ135_3);
    client.publish("sensor/mq135_3", payload);
    
    snprintf(payload, sizeof(payload), "%.2f", MQ4_1);
    client.publish("sensor/mq4_1", payload);
    
    snprintf(payload, sizeof(payload), "%.2f", MQ4_2);
    client.publish("sensor/mq4_2", payload);
    
    snprintf(payload, sizeof(payload), "%.2f", MQ4_3);
    client.publish("sensor/mq4_3", payload);
    
    snprintf(payload, sizeof(payload), "%.2f", tekanan1);
    client.publish("sensor/tekanan1", payload);
    
    snprintf(payload, sizeof(payload), "%.2f", tekanan2);
    client.publish("sensor/tekanan2", payload);
    
    snprintf(payload, sizeof(payload), "%.2f", tekanan3);
    client.publish("sensor/tekanan3", payload);
    
    snprintf(payload, sizeof(payload), "%.2f", suhu[0]);
    client.publish("sensor/temp1", payload);
    
    snprintf(payload, sizeof(payload), "%.2f", suhu[1]);
    client.publish("sensor/temp2", payload);
    
    snprintf(payload, sizeof(payload), "%.2f", suhu[2]);
    client.publish("sensor/temp3", payload);
    
    snprintf(payload, sizeof(payload), "%.2f", suhu_dht);
    client.publish("sensor/dht_temp", payload);
    
    snprintf(payload, sizeof(payload), "%.2f", kelembaban);
    client.publish("sensor/dht_hum", payload);

    kirimData = true;
}

void logData() {
    // Format data sensor yang akan disimpan
    String data = String(millis()) + "," +
                  String(MQ135_1) + "," + String(MQ135_2) + "," + String(MQ135_3) + "," +
                  String(MQ4_1) + "," + String(MQ4_2) + "," + String(MQ4_3) + "," +
                  String(tekanan1) + "," + String(tekanan2) + "," + String(tekanan3) + "," +
                  String(suhu[0]) + "," + String(suhu[1]) + "," + String(suhu[2]) + "," +
                  String(suhu_dht) + "," + String(kelembaban);

    // Cek apakah file "data.csv" sudah ada
    if (!SD.exists("/data.csv")) {
        Serial.println("File data.csv tidak ditemukan, membuat file baru...");
        
        // Buat file baru dengan header kolom
        File file = SD.open("/data.csv", FILE_WRITE);
        if (file) {
            file.println("Time(ms),MQ135_1,MQ135_2,MQ135_3,MQ4_1,MQ4_2,MQ4_3,Pressure1,Pressure2,Pressure3,Temp1,Temp2,Temp3,DHT_Temp,Humidity");
            file.close();
        } else {
            Serial.println("Gagal membuat file data.csv!");
        }
    }

    // Buka file dalam mode append untuk menambahkan data baru
    File file = SD.open("/data.csv", FILE_APPEND);
    if (file) {
        file.println(data);
        file.flush();
        file.close();
        Serial.println("Data berhasil disimpan: " + data);
    } else {
        Serial.println("Gagal menyimpan data ke data.csv!");
    }
}


void reconnect() {
    while (!client.connected()) {
        Serial.print("Connecting to MQTT...");
        if (client.connect("ESP32Client")) {
            Serial.println("Connected!");
            wifiStatus = true;
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" Retrying in 5 seconds...");
            delay(5000);
        }
    }
}

// float readMPX(int pin, Ewma &filter) {
//     float raw = analogRead(pin);
//     Serial.println(analogRead(pin));
//     float filtered = filter.filter(raw);
//     float V_adc = (filtered * 3.3) / 4095;
//     float V_sensor = (V_adc / 3.3 )* DIVIDER_RATIO / 0.0012858;
//     float calibrated_pressure = (0.0009 * V_sensor * V_sensor) + (2.2194 * V_sensor) + 2.0596;
//     return raw < 0 ? 0 : raw;
// }

float readMPX(int pin, Ewma &filter) {
    
    float raw = analogRead(pin);
    float filtered = filter.filter(raw);
    float kPa = (3E-05 * filtered * filtered) + (0.0964 * filtered) - 61.954;     
    return kPa;
}

float mq135Calb(float voltage) {
    return (3601.9 * voltage * voltage) - (1190.6 * voltage) + 461.63;
}

float readVoltage(Adafruit_ADS1115 &ads, int channel) {
    int16_t adc = ads.readADC_SingleEnded(channel);
    // Serial.println(adc);
    return (adc * 0.125) / 1000.0; // Konversi ke volt
}

void calibrateSensor(MQUnifiedsensor &sensor, Adafruit_ADS1115 &ads, uint8_t channel, int index) {
    float calcR0 = 0;
    float offsetSum = 0;
    
    for (int i = 0; i < 10; i++) {
        float voltage = readVoltage(ads, channel);
        sensor.externalADCUpdate(voltage);
        calcR0 += sensor.calibrate(RATIO_MQ4_CLEAN_AIR);
        Serial.print(".");
    }
    sensor.setR0(calcR0 / 10);
    
    if (isinf(calcR0) || calcR0 == 0) {
        Serial.println("Error: Check wiring!");
        while (1);
    }
    
    // Hitung offset untuk baseline udara bersih
    for (int i = 0; i < OFFSET_SAMPLES; i++) {
        offsetSum += calculateGas(sensor, readVoltage(ads, channel));
        delay(10);
    }
    baselineOffset[index] = offsetSum / OFFSET_SAMPLES;
}

float calculateGas(MQUnifiedsensor &sensor, float voltage) {
    sensor.externalADCUpdate(voltage);
    sensor.setA(A_METHANE);
    sensor.setB(B_METHANE);
    return sensor.readSensor();
}
