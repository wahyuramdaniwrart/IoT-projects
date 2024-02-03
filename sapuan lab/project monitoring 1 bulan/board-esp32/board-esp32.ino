


// RTC
#include <RTClib.h>
#include <Wire.h>

#define SDA 21
#define SCL 22

RTC_DS3231 rtc;


// MEMORY
#include "FS.h"
#include <SD.h>
#include "SPI.h"

#define SD_CS 2
String data_save;

// MILLIS
unsigned long interval   = 3000;   //waktu realtime
unsigned long waktuAwal  = 0;

unsigned long interval2  = 6000; //waktu penyimpanan
unsigned long waktuAwal2 = 0;

byte tahun;
byte bulan;
byte tanggal;
byte hari;
byte jam;
byte menit;
byte detik;
bool h24;
bool PM;
boolean century = false;


// blynk

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#define BLYNK_PRINT Serial
// #define USE_NODE_MCU_BOARD
#define BLYNK_TEMPLATE_ID "TMPL6hntJtddh"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "DlhXkzCYdTqU0yy38MGArcvqOB514naJ"



// Your WiFi credentials.
// Set password to "" for open networks.
char auth[] = BLYNK_AUTH_TOKEN ;  // Ganti dengan token Blynk Anda
char ssid[] = "babyfleur";
char pass[] = "babyfleur";

// TERIMA DATA

const char endMarker = '#';
const int numSensors = 4;  // Jumlah sensor
char receivedChars[numSensors][20];  // Array untuk masing-masing sensor
bool newData[numSensors] = {false, false, false, false};  // Array flag newData untuk masing-masing sensor

// Variable untuk menyimpan nilai dari masing-masing sensor
float pressureValue = 0;
float temperatureValue = 0;
float methaneValue = 0;
float co2Value = 0;

//Google Sheet
#include <GSheet32.h>
GSheet32 Sheet("AKfycby5AJ5PUhqtz7HA0hTYfIU0_w8UzaD1-M--2t_i-17gZjI0IPCPbVcHAjyRBpuw_fIf");



/////////////////////////////////////////////////


void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file) {
    Serial.println("Failed to open file for appending");
  }
  if(file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void setup() {
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass );
  Wire.begin();

  // RTC
  rtc.begin();
  // rtc.adjust(DateTime(F(__DATE__),F(__TIME__))); 
  // rtc.adjust(DateTime(2024, 01, 31, 19, 42, 00));



 // MEMORY
 SD.begin(SD_CS);
 if (!SD.begin(SD_CS)) {
   Serial.println("Gagal Memuat Kartu SD");
 }
 uint8_t cardType = SD.cardType();
 if (cardType == CARD_NONE) {
   Serial.println("Tidak Ada Kartu SD");
 } else {

   Serial.println("Kartu SD ditemukan...");
   Serial.println("Menginisialisasi kartu SD...");

   File file = SD.open("/monitoring.csv");
   if(!file) {
     Serial.println("File tidak ditemukan");
     Serial.println("Membuat file...");
     writeFile(SD, "/monitoring.csv", "tekanan, suhu, metana, carbondioksida, waktu, tanggal \n");
   }
   else {
     Serial.println("File sudah tersedia :) ");  
   }
   file.close();
     
 }
   

}

void loop() {
  unsigned long waktuSekarang = millis();
  unsigned long waktuSekarang2 = millis();

  Blynk.run();

  // WAKTU 1
  if (waktuSekarang - waktuAwal >= interval) {
    
    
    BLYNK();
    nilaiWaktu();

    for (int i = 0; i < numSensors; i++) {
        recvWithEndMarker(i);
        showNewData(i);
    }

    // showSensorValues();

    waktuAwal = waktuSekarang;
  }

  
  

  if (waktuSekarang2 - waktuAwal2 >= interval2) {

  //  memori card

   DateTime now = rtc.now();

   data_save = String(pressureValue) + " , " + String(temperatureValue) + " , " + String(methaneValue) + " , " + String(co2Value) + " , " + 
               String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + " , " + 
               String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + "\n";
   appendFile(SD, "/monitoring.csv", data_save.c_str());

  

  //  google sheet

  // Sheet.sendData(String(pressureValue), String(temperatureValue), String(methaneValue),String(co2Value));

  //   waktuAwal2 = waktuSekarang2;
  }
  

}



void recvWithEndMarker(int sensorIndex) {
    static byte ndx = 0;
    char rc;

    while (Serial.available() > 0 && newData[sensorIndex] == false) {
        rc = Serial.read();

        if (rc != endMarker) {
            receivedChars[sensorIndex][ndx] = rc;
            ndx++;
            if (ndx >= sizeof(receivedChars[sensorIndex]) - 1) {
                ndx = sizeof(receivedChars[sensorIndex]) - 2;
            }
        } else {
            receivedChars[sensorIndex][ndx] = '\0'; // terminate the string
            ndx = 0;
            newData[sensorIndex] = true;
        }
    }
}


void showSensorValues() {
    // Menampilkan nilai semua sensor ke layar
    Serial.print("Pressure: ");
    Serial.println(pressureValue);
    Serial.print("Temperature: ");
    Serial.println(temperatureValue);
    Serial.print("Methane: ");
    Serial.println(methaneValue);
    Serial.print("CO2: ");
    Serial.println(co2Value);
}

void showNewData(int sensorIndex) {
    if (newData[sensorIndex]) {
        // Serial.print("Sensor ");
        // Serial.print(sensorIndex + 1);  // Menampilkan nomor sensor
        // Serial.print(": ");
        // Serial.println(receivedChars[sensorIndex]);

        // Memanggil fungsi konversi dan penugasan nilai
        convertAndAssignValues(sensorIndex);

        newData[sensorIndex] = false;
    }
}

void convertAndAssignValues(int sensorIndex) {
    // Konversi nilai dari karakter menjadi tipe data yang sesuai
    switch (sensorIndex) {
        case 0:  // Sensor tekanan
            pressureValue = atof(receivedChars[sensorIndex]);
            break;
        case 1:  // Sensor suhu
            temperatureValue = atof(receivedChars[sensorIndex]);
            break;
        case 2:  // Sensor metana
            methaneValue = atof(receivedChars[sensorIndex]);
            break;
        case 3:  // Sensor karbon dioksida
            co2Value = atof(receivedChars[sensorIndex]);
            break;
    }
}


void BLYNK() {

  Blynk.virtualWrite(V1, pressureValue);  // Pin virtual V0 untuk tekanan
  Blynk.virtualWrite(V2, temperatureValue);  // Pin virtual V1 untuk suhu
  Blynk.virtualWrite(V3, methaneValue);  // Pin virtual V2 untuk metana
  Blynk.virtualWrite(V4, co2Value);  // Pin virtual V3 untuk karbon dioksida
}

void nilaiWaktu() {
    DateTime now = rtc.now();
      
    Serial.print(data_save);

}
