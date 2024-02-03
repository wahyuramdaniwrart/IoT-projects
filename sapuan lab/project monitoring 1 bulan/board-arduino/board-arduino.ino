

// MPX5700DP
#include <Ewma.h>

Ewma adcFilter1(0.1);   // Less smoothing - faster to detect changes, but more prone to noise

#define TC_PIN A0         // set to ADC pin used
#define AREF 5.0           // set to AREF, typically board voltage like 3.3 or 5.0
#define ADC_RESOLUTION 10  // set to ADC bit resolution, 10 is default

int analog_mpx;
float tegangan_mpx;
float tekanan;
float tekanan_calb;
float tekanan_ofset_lcd;

float get_voltage(int raw_adc) {
  return raw_adc * (AREF / (pow(2, ADC_RESOLUTION) - 1));
}

// DS18B20
#include <DallasTemperature.h>
#include <OneWire.h>

#define ONE_WIRE_BUS A1
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensorSuhu(&oneWire); 
float suhu;

// RTC
#include <RTClib.h>
#include <Wire.h>

RTC_DS3231 rtc;


// MEMORY
#include <FS.h>
#include <FSImpl.h>
#include <vfs_api.h>
#include <SD.h>
#include "SPI.h"

#define SD_CS 2
String data_save;

// MQ-sensor

#include <MQUnifiedsensor.h>
#include <ArduinoUnit.h>

#define         Board                   ("Arduino Uno")
#define         Pin1                     (A2)  //Analog input 2 of your arduino
#define         Pin2                     (A3)  //Analog input 3 of your arduino
#define         RatioMQ4CleanAir          (4.4) //RS / R0 = 4.4 ppm 
#define         RatioMQ135CleanAir        (3.6) //RS / R0 = 9.6 ppm 
#define         ADC_Bit_Resolution        (10) // 10 bit ADC 
#define         Voltage_Resolution        (5) // Volt resolution to calc the voltage
#define         Type                      ("Arduino Uno") //Board used

// define

MQUnifiedsensor MQ4(Board, Voltage_Resolution, ADC_Bit_Resolution, Pin1, Type);
MQUnifiedsensor MQ135(Board, Voltage_Resolution, ADC_Bit_Resolution, Pin2,Type);


// MILLIS
unsigned long interval   = 3000;   //waktu realtime
unsigned long waktuAwal  = 0;

unsigned long interval2  = 600000; //waktu penyimpanan
unsigned long waktuAwal2 = 0;

// Include application, user and local libraries
#include "SPI.h"
#include "TFT_22_ILI9225.h"

// Include font definition files
// NOTE: These files may not have all characters defined! Check the GFXfont def
// params 3 + 4, e.g. 0x20 = 32 = space to 0x7E = 126 = ~

#include <../fonts/FreeSans12pt7b.h>
#include <../fonts/FreeSans24pt7b.h>

#define TFT_RST 8
#define TFT_RS  9
#define TFT_CS  10  // SS
#define TFT_SDI 11  // MOSI
#define TFT_CLK 13  // SCK
#define TFT_LED 0   // 0 if wired to +5V directly

#define TFT_BRIGHTNESS 200 // Initial brightness of TFT backlight (optional)

// Use hardware SPI (faster - on Uno: 13-SCK, 12-MISO, 11-MOSI)
TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_LED, TFT_BRIGHTNESS);
// Use software SPI (slower)
//TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_SDI, TFT_CLK, TFT_LED, TFT_BRIGHTNESS);

// Variables and constants
uint16_t x, y;
boolean flag = false;

// kirim data

#define MAX_REQUEST_LENGTH 10
String minta = "";

////////////////////////////////////////////////////////////////////////////////

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

// ================================================>>>>>

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

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

  //DS18B20----------------------------------------------
  sensorSuhu.begin();

  // MQ-SENSOR--------------------------------------------
  MQ4.init(); 
  MQ4.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ4.setA(1012.7); MQ4.setB(-2.786); // Configure the equation to to calculate CH4 concentration
  MQ4.setR0(19);
  
  MQ135.init(); 
  MQ135.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ135.setA(110.47); MQ135.setB(-2.862); // Configure the equation to to calculate LPG concentration
  MQ135.setR0(7.71);

//  Serial.print("Calibrating please wait.");
  float MQ4calcR0 = 0, 
  MQ135calcR0 = 0;

  for(int i = 1; i<=10; i ++)
  {
    //Update the voltage Values
    MQ4.update();
    MQ135.update();

    MQ4calcR0 += MQ4.calibrate(RatioMQ4CleanAir);
    MQ135calcR0 += MQ135.calibrate(RatioMQ135CleanAir);

    // Serial.print(".");
  }
  MQ4.setR0(MQ4calcR0/20);
  MQ135.setR0(MQ135calcR0/20);
  // Serial.println("  done!.");
  
  // Serial.print("Valores de R0 para cada sensor (MQ4 & MQ135):");
  // Serial.print(MQ4calcR0/10); Serial.print(" | ");
  // Serial.print(MQ135calcR0/10); Serial.print(" | ");

  if(isinf(MQ4calcR0) || isinf(MQ135calcR0) ){Serial.println("Warning: Conection issue founded, R0 is infite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(MQ4calcR0 == 0 || MQ135calcR0 == 0){Serial.println("Warning: Conection issue founded, R0 is zero (Analog pin with short circuit to ground) please check your wiring and supply"); while(1);}
  /*****************************  MQ CAlibration ********************************************/ 
 
  // //Print in serial monitor
  // Serial.println("MQ4 to MQ135 - lecture");
  // Serial.println("*************************** Values from MQ-board ***************************");
  // Serial.println("|    metana   |  CO2 |");  
  // Serial.println("|    MQ-4  |   MQ-135   |");  
  // //pinMode(calibration_button, INPUT);

  // LCD----------------------------------------------------------------------
  tft.begin();
  tft.clear();
}

void loop() {
  unsigned long waktuSekarang = millis();
  unsigned long waktuSekarang2 = millis();

  // WAKTU 1
  if (waktuSekarang - waktuAwal >= interval) {
    
    TEKANAN();
    SUHU();
    LCD();
    KIRIM();
    readAllSensors();
    dataLogger();

    waktuAwal = waktuSekarang;
  }

    // waktukirim();
    // KIRIM();

    // delay (600);

}


////////////////////////////////////////////////////////////////////////////////

void waktukirim() {

 while (Serial.available() > 0) {
    char receivedChar = Serial.read();

    // Periksa apakah karakter yang diterima adalah karakter newline atau batas panjang string
    if (receivedChar == '\n' || minta.length() >= MAX_REQUEST_LENGTH) {
      minta.trim(); // Hilangkan spasi di awal dan akhir string

      if (minta == "Ya") {
        KIRIM();
      }

      // Reset string minta setelah diproses
      minta = "";
    } else {
      minta += receivedChar;
    }
  }
}

void TEKANAN() {
  // tekanan
  analog_mpx = analogRead(TC_PIN);
  // float filtered1 = adcFilter1.filter(analog_mpx);
  tegangan_mpx = (analog_mpx * 5.0) / 1023.0;
  // tegangan_mpx = get_voltage(filtered1);
  tekanan = ((tegangan_mpx / 5.0) - 0.04) / 0.0012858;
  tekanan_calb = tekanan;
  if (tekanan_calb < 0.2) {
    tekanan_calb = 0;
  }

  // Serial.print("tekanan :");
  // Serial.println(String(tekanan_calb));
}

void SUHU() {
    // SUHU
  sensorSuhu.requestTemperatures();
  suhu = sensorSuhu.getTempCByIndex(0);
  // Serial.print("suhu :");
  // Serial.println(suhu);
}

void readAllSensors()
{
  //Update the voltage Values
  MQ4.update();
  MQ135.update();

  float MQ4Lecture =  MQ4.readSensor();
  float MQ135Lecture =  MQ135.readSensor();
  //Read the sensor and print in serial port

  // Serial.print("| "); Serial.print(MQ4Lecture);
  // Serial.print("   |   "); Serial.print(MQ135Lecture);
  // Serial.println("|");
}

void KIRIM () {

  String datakirim = String(tekanan_calb) + "#" + String(suhu) + "#" + String(MQ4.readSensor()) + "#" + String(MQ135.readSensor()) + "#";

  Serial.println(datakirim);

  /////////////////////////

  // Serial.println(tekanan_calb);
  // Serial.println(suhu);
  // Serial.println(MQ4.readSensor());
  // Serial.println(MQ135.readSensor());

}

void dataLogger() {

  //  memori card

   DateTime now = rtc.now();

   data_save = String(tekanan_calb) + " , " + String(suhu) + " , " + String(MQ4.readSensor()) + " , " + String(MQ135.readSensor()) + " , " + 
               String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + " , " + 
               String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + "\n";
   appendFile(SD, "/monitoring.csv", data_save.c_str());

}

void LCD () {
  tft.setOrientation(3);  


  tft.drawText(0, 0, "TEKANAN", COLOR_CYAN );
  tft.setBackgroundColor(COLOR_BLACK);
  tft.setFont(Terminal12x16);
  
  tft.drawText(0, 20, "SUHU", COLOR_CYAN );
  tft.setBackgroundColor(COLOR_BLACK);
  tft.setFont(Terminal12x16);

  tft.drawText(0, 50, "C02 1", COLOR_CYAN );
  tft.setBackgroundColor(COLOR_BLACK);
  tft.setFont(Terminal12x16);

  tft.drawText(0, 100, "METANA 1", COLOR_CYAN );
  tft.setBackgroundColor(COLOR_BLACK);
  tft.setFont(Terminal12x16);

  // ---------------------------------------------
      String tekanan_lcd = String(tekanan_calb);
      String suhu_lcd = String(suhu);
      String carbon_1_lcd = String(MQ135.readSensor());
      String metana_1_lcd = String(MQ4.readSensor());

  // ---------------------------------------------

  tft.drawText(110, 0, tekanan_lcd, COLOR_GREEN );
  tft.setBackgroundColor(COLOR_BLACK);
  tft.setFont(Terminal12x16);
  
  tft.drawText(110, 20, suhu_lcd, COLOR_GREEN );
  tft.setBackgroundColor(COLOR_BLACK);
  tft.setFont(Terminal12x16);

  tft.drawText(110, 50, carbon_1_lcd, COLOR_GREEN );
  tft.setBackgroundColor(COLOR_BLACK);
  tft.setFont(Terminal12x16);

  tft.drawText(110, 100, metana_1_lcd, COLOR_GREEN );
  tft.setBackgroundColor(COLOR_BLACK);
  tft.setFont(Terminal12x16);
}
