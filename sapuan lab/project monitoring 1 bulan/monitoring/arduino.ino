#include "TFT_22_ILI9225.h"
#include <SPI.h>
#include <RTClib.h>
#include <SD.h>
#include <Wire.h>
#include <DHT11.h>
#include <MQUnifiedsensor.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Ewma.h>

// Pin definitions
#define TFT_RST 48
#define TFT_RS 49
#define TFT_CS 47
#define TFT_SDI 51
#define TFT_CLK 52
#define TFT_LED 0
#define TFT_BRIGHTNESS 200
#define CS_PIN 53
#define ONE_WIRE_BUS_1 A5
#define ONE_WIRE_BUS_2 A6
#define DHT_PIN A4
#define TC_PIN1 A2
#define TC_PIN2 A3
#define MQ4_PIN1 A7
#define MQ4_PIN2 A8
#define MQ135_PIN1 A9
#define MQ135_PIN2 A10
#define AREF 5.0
#define ADC_RESOLUTION 10
#define BOARD "Arduino Uno"
#define VOLTAGE_RESOLUTION 5

// LCD initialization
TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, TFT_LED);

// RTC and SD card
RTC_DS1307 rtc;
File dataFile;
String tanggal = "";
String waktu = "";

// DHT11 sensor
DHT11 dht(DHT_PIN);
int temperature = 0;
int humidity = 0;
int result = 0;

// MQ sensors
MQUnifiedsensor MQ4_1(BOARD, VOLTAGE_RESOLUTION, ADC_RESOLUTION, MQ4_PIN1, "MQ-4");
MQUnifiedsensor MQ4_2(BOARD, VOLTAGE_RESOLUTION, ADC_RESOLUTION, MQ4_PIN2, "MQ-4");
MQUnifiedsensor MQ135_1(BOARD, VOLTAGE_RESOLUTION, ADC_RESOLUTION, MQ135_PIN1, "MQ-135");
MQUnifiedsensor MQ135_2(BOARD, VOLTAGE_RESOLUTION, ADC_RESOLUTION, MQ135_PIN2, "MQ-135");
float MQ4Lecture1, MQ4Lecture2, MQ135Lecture1, MQ135Lecture2;

// DS18B20 sensors
OneWire oneWire1(ONE_WIRE_BUS_1);
OneWire oneWire2(ONE_WIRE_BUS_2);
DallasTemperature sensorSuhu1(&oneWire1);
DallasTemperature sensorSuhu2(&oneWire2);
float suhu1, suhu2;

// MPX5700AP sensors
Ewma adcFilter1(0.1);
Ewma adcFilter2(0.1);
float tegangan_mpx1, tegangan_mpx2, tekanan1, tekanan2, tekanan_calb1, tekanan_calb2;
int analog_mpx1, analog_mpx2;

float get_voltage(int raw_adc)
{
    return raw_adc * (AREF / (pow(2, ADC_RESOLUTION) - 1));
}

// Function declarations
void initializeSystem();
bool checkSensors();
void displayLog(const String &message);
void readSensors();
void readRTC();
void readDHT();
void readPressure();
void readTemperature();
void saveToSDCard();
void displayDataOnLCD();
float getVoltage(int raw_adc);
void initializeMQSensor(MQUnifiedsensor &sensor, float ratioCleanAir, float a, float b, float r0);
void KIRIM();
void waktukirim();
void handleSDCardRemoval();
float readAndAverageSensor(int pin, int numReadings);
float readAndFilterMQ(MQUnifiedsensor &sensor, int numReadings);

// Variable to keep track of the current line for logging
int currentLine = 0;
unsigned long lastSaveTime = 0; // Variable to track the last save time

void setup()
{
    Serial.begin(115200);
    tft.begin();
    tft.setOrientation(1); // Set to landscape orientation
    tft.setBacklight(true);
    tft.setFont(Terminal6x8);
    // tft.clear();
    tft.setBackgroundColor(COLOR_BLACK);
    tft.drawText(0, 0, "Initializing...");

    initializeSystem();

    if (checkSensors())
    {
        displayLog("All sensors and SD card ready.");
        tft.clear();
    }
    else
    {
        displayLog("Initialization failed. Check sensors and SD card.");
        tft.clear();
        while (true)
            ;
    }
}

void loop()
{
    readRTC();
    unsigned long currentMillis = millis();
    if (currentMillis - lastSaveTime >= 3600000)
    { // 1 hours = 3,600,000 milliseconds
        saveToSDCard();
        lastSaveTime = currentMillis;
    }
    readSensors();
    displayDataOnLCD();
    KIRIM();
    waktukirim();
    delay(1000);
}

void initializeSystem()
{
    rtc.begin();
    if (!rtc.isrunning())
    {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    pinMode(CS_PIN, OUTPUT);
    digitalWrite(CS_PIN, HIGH);

    if (!SD.begin())
    {
        displayLog("SD card initialization failed.");
        while (true)
            ;
    }

    dataFile = SD.open("datalog.csv", FILE_WRITE);
    if (!dataFile)
    {
        displayLog("Failed to open datalog.csv.");
        while (true)
            ;
    }

    sensorSuhu1.begin();
    sensorSuhu2.begin();

    // Initialize MQ Sensors
    initializeMQSensor(MQ4_1, 4.4, 1012.7, -2.786, 10.0);
    initializeMQSensor(MQ4_2, 4.4, 1012.7, -2.786, 10.0);
    initializeMQSensor(MQ135_1, 3.6, 110.47, -2.862, 10.0);
    initializeMQSensor(MQ135_2, 3.6, 110.47, -2.862, 10.0);
}

bool checkSensors()
{
    bool allSensorsReady = true;

    // if (!rtc.begin()) {
    //   displayLog("RTC not found.");
    //   allSensorsReady = false;
    // }

    if (!SD.begin(CS_PIN))
    {
        displayLog("SD card not found.");
        allSensorsReady = false;
    }

    return allSensorsReady;
}

void displayLog(const String &message)
{
    tft.drawText(0, currentLine * 16, "                    "); // Clear previous data
    tft.drawText(0, currentLine * 16, message.c_str(), COLOR_WHITE);

    currentLine++;
    if (currentLine >= 14)
    {
        currentLine = 0;
    }
}

void readSensors()
{
    readDHT();
    readPressure();
    readTemperature();

    // Read and display CO2 and CH4 concentrations from MQ sensors
    MQ135Lecture1 = readAndFilterMQ(MQ135_1, 10);
    MQ135Lecture2 = readAndFilterMQ(MQ135_2, 10);
    MQ4Lecture1 = readAndFilterMQ(MQ4_1, 10);
    MQ4Lecture2 = readAndFilterMQ(MQ4_2, 10);
}

void readRTC()
{
    DateTime now = rtc.now();
    tanggal = String(now.day()) + "/" + String(now.month()) + "/" + String(now.year());
    waktu = String(now.hour()) + ":" + String(now.minute());
}

void readDHT()
{
    result = dht.readTemperatureHumidity(temperature, humidity);
    //   temperature = readAndAverageSensor(DHT_PIN, 10);
    //   humidity = readAndAverageSensor(DHT_PIN, 10);
}

void readPressure()
{
    // Baca nilai analog dari sensor tekanan 1
    analog_mpx1 = analogRead(TC_PIN1);
    float filtered1 = adcFilter1.filter(analog_mpx1);
    tegangan_mpx1 = get_voltage(filtered1);
    tekanan1 = ((tegangan_mpx1 / 5.0) - 0.04) / 0.0012858;
    tekanan_calb1 = tekanan1 - 99.65;
    if (tekanan_calb1 < 0.2)
    {
        tekanan_calb1 = 0;
    }

    // Baca nilai analog dari sensor tekanan 2
    analog_mpx2 = analogRead(TC_PIN2);
    float filtered2 = adcFilter2.filter(analog_mpx2);
    tegangan_mpx2 = get_voltage(filtered2);
    tekanan2 = ((tegangan_mpx2 / 5.0) - 0.04) / 0.0012858;
    tekanan_calb2 = tekanan2 - 98.89;
    if (tekanan_calb2 < 0.2)
    {
        tekanan_calb2 = 0;
    }

    // tekanan1 = readAndAverageSensor(TC_PIN1, 10);
    // tekanan2 = readAndAverageSensor(TC_PIN2, 10);

    // tekanan_calb1 = tekanan1 - 171;
    // if (tekanan_calb1 < 0.2) tekanan_calb1 = 0;

    // tekanan_calb2 = tekanan2 - 171;
    // if (tekanan_calb2 < 0.2) tekanan_calb2 = 0;
}

void readTemperature()
{
    sensorSuhu1.requestTemperatures();
    suhu1 = sensorSuhu1.getTempCByIndex(0);

    sensorSuhu2.requestTemperatures();
    suhu2 = sensorSuhu2.getTempCByIndex(0);
}

void saveToSDCard()
{
    if (!SD.exists("datalog.csv"))
    {
        handleSDCardRemoval();
        return;
    }

    String dataSave = waktu + " , " + tanggal + " , " + String(temperature) + " , " + String(humidity) + " , " + String(tekanan_calb1) + " , " + String(tekanan_calb2) + " , " + String(suhu1) + " , " + String(suhu2) + " , " + String(MQ4Lecture1) + " , " + String(MQ4Lecture2) + " , " + String(MQ135Lecture1) + " , " + String(MQ135Lecture2) + "\n";

    if (dataFile)
    {
        dataFile.println(dataSave);
        dataFile.flush();
    }
    else
    {
        dataFile = SD.open("datalog.csv", FILE_WRITE);
        if (!dataFile)
        {
            handleSDCardRemoval();
        }
    }
}

void handleSDCardRemoval()
{
    tft.clear();
    tft.setFont(Terminal12x16); // Set a larger font for the warning message
    tft.drawText(10, 10, "Warning:", COLOR_RED);
    tft.drawText(10, 30, "SD card removed!", COLOR_RED);
    tft.drawText(10, 50, "System halted.", COLOR_RED);
    while (true)
        ; // Halt the system
}

void displayDataOnLCD()
{
    // tft.clear();
    tft.setFont(Terminal12x16);                            // Set a larger font for date and time
    tft.drawText(20, 5, waktu + " " + tanggal, COLOR_RED); // Use a different color for date and time

    tft.setFont(Terminal6x8); // Reset font for sensor data
    tft.drawText(25, 30, "Temp: " + String(temperature) + " C" + " | "
                                                                 "Humidity :" +
                             String(humidity) + " %",
                 COLOR_YELLOWGREEN);

    tft.drawText(0, 50, "Pressure1 (kPa)", COLOR_WHITE);
    tft.drawText(120, 50, "Pressure2 (kPa)", COLOR_WHITE);

    tft.drawText(0, 80, "Suhu 1 (C)", COLOR_WHITE);
    tft.drawText(120, 80, "Suhu 2 (C)", COLOR_WHITE);

    tft.drawText(0, 110, "CH4_1 (ppm)", COLOR_WHITE);
    tft.drawText(120, 110, "CH4_2 (ppm)", COLOR_WHITE);

    tft.drawText(0, 140, "CO2_1 (ppm)", COLOR_WHITE);
    tft.drawText(120, 140, "CO2_2 (ppm)", COLOR_WHITE);

    tft.setFont(Terminal11x16);
    tft.drawText(0, 60, String(tekanan_calb1), COLOR_GREEN);
    tft.drawText(120, 60, String(tekanan_calb2), COLOR_GREEN);

    tft.drawText(0, 90, String(suhu1), COLOR_GREEN);
    tft.drawText(120, 90, String(suhu2), COLOR_GREEN);

    tft.drawText(0, 120, String(MQ4Lecture1), COLOR_GREEN);
    tft.drawText(120, 120, String(MQ4Lecture2), COLOR_GREEN);

    tft.drawText(0, 150, String(MQ135Lecture1), COLOR_GREEN);
    tft.drawText(120, 150, String(MQ135Lecture2), COLOR_GREEN);
}

// float getVoltage(int raw_adc) {
//   return (raw_adc * (5.0 / 1023.0));
// }

void initializeMQSensor(MQUnifiedsensor &sensor, float ratioCleanAir, float a, float b, float r0)
{
    sensor.setRegressionMethod(1);
    sensor.setA(a);
    sensor.setB(b);
    sensor.setR0(r0);
    sensor.init();
    float calcR0 = 0;
    for (int i = 1; i <= 10; i++)
    {
        sensor.update();
        calcR0 += sensor.calibrate(ratioCleanAir);
    }
    sensor.setR0(calcR0 / 10);
    if (isinf(calcR0))
    {
        Serial.println("Warning: Connection issue detected with MQ sensor.");
        while (true)
            ;
    }
    if (calcR0 == 0)
    {
        Serial.println("Warning: Connection issue detected with MQ sensor.");
        while (true)
            ;
    }
}

void KIRIM()
{
    // Implement the function to send data via GSM or other means
    float sensorValues[8] = {tekanan_calb1, tekanan_calb2, suhu1, suhu2, MQ4Lecture1, MQ4Lecture2, MQ135Lecture1, MQ135Lecture2};

    String dataToSend = "<";
    for (int i = 0; i < 8; i++)
    {
        dataToSend += String(sensorValues[i]);
        if (i < 7)
        {
            dataToSend += ",";
        }
    }
    dataToSend += ">";

    Serial.println(dataToSend);
}

void waktukirim()
{
    // Implement the function to handle timing for data transmission
    while (Serial.available() > 0)
    {
        Serial.read();
        KIRIM();
    }
}

float readAndAverageSensor(int pin, int numReadings)
{
    float total = 0;
    for (int i = 0; i < numReadings; i++)
    {
        total += analogRead(pin);
        delay(10); // Small delay between readings to allow sensor to stabilize
    }
    return total / numReadings;
}

float readAndFilterMQ(MQUnifiedsensor &sensor, int numReadings)
{
    float total = 0;
    for (int i = 0; i < numReadings; i++)
    {
        sensor.update();
        total += sensor.readSensor();
        delay(10); // Small delay between readings to allow sensor to stabilize
    }
    return total / numReadings;
}
