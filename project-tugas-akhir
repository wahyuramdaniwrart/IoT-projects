#include <Adafruit_ADS1X15.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EmonLib.h>
#include <SdFat.h>
#include <RTClib.h>
#include <ESP8266WebServer.h>  // Include HTTP server library

// Create an HTTP server on port 80
ESP8266WebServer server(80);

RTC_DS1307 rtc;

// define blynk
#define BLYNK_TEMPLATE_ID "TMPL6vZxZHWhR"
#define BLYNK_TEMPLATE_NAME "TUGAS AKHIR"
#define BLYNK_AUTH_TOKEN "C7OeG8dh2-L9A1vd2VF2yKjJgTPYZV8Y"
/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
/* Fill in information from Blynk Device Info here */
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
// Your WiFi credentials.
char ssid[] = "@unram.ac.id";
char pass[] = "";

// SD card setup
#define SD_CS 10
SdFat sd;
SdFile dataFile;
bool sdReady = false;
bool virtualButtonState = false;  // To track the state of the Blynk button
// Flag untuk mencatat apakah tombol ON di Blynk sudah ditekan
bool headerWritten = false;  // Flag untuk mengecek apakah header telah ditulis


// Initialize SCT-019 (AC current sensor)
int sct_pin = A0;  // Pin for AC current sensor
double Irms, prevIrms = -1;  // AC current
EnergyMonitor emon1;

// Initialize ADS1115 (DC voltage and current readings)
Adafruit_ADS1115 ads1;
Adafruit_ADS1115 ads2;

//definisi stopwatch

float startTime = 0;  // Waktu mulai stopwatch
float elapsedTime = 0;  // Waktu yang telah berlalu


float voltage = 0;
float vbat = 0;
float R1 = 220000.0, R2 = 10000.0;
float voltageMultiplier = (R1 + R2) / R2;  // Voltage divider factor for DC voltage
int16_t results;
float rShunt = 0.25;  // For 300A 75mV shunt resistor
float arus = 0;
float vShunt = 0;
float multiplier = 0.0078125;

// 3 phase voltage

float VoltageReadOffset = 0.0;
float Voltage = 0.0;
float Volt_ref = 0;
float calibrasi;

int gnd3 = D3;
int gnd4 = D4;

// Timing variables
unsigned long currentMillis = millis();
static unsigned long lastUpdateTime = 0;
static unsigned long lastSaveTime = 0;

// Function to check the SD card status
bool checkSdCard() {
  SdSpiConfig spiConfig(SD_CS, SHARED_SPI, SD_SCK_MHZ(16));

  if (sd.begin(spiConfig)) {
    Serial.println("SD card initialized successfully.");
    return true;  // SD card is ready
  } else {
    Serial.println("Failed to initialize SD card. Retrying...");
    return false;  // SD card is not ready
  }
}

// Virtual button handler
BLYNK_WRITE(V4) {
  virtualButtonState = param.asInt();  // Get the state of the virtual button (ON/OFF)

  if (virtualButtonState == 1) {  // Jika tombol di Blynk di-klik ON
    headerWritten = false;  // Reset flag header saat tombol dihidupkan
    startTime = millis();  // Catat waktu mulai
    Serial.println("Blynk Button ON: Start recording and stopwatch.");
  } else {
    // Hitung waktu yang telah berlalu
    unsigned long elapsedTime = millis() - startTime;

    // Konversi waktu ke detik
    float elapsedSeconds = elapsedTime / 1000.0;

    // Tambahkan satuan ke dalam string
    String timeWithUnit = String(elapsedSeconds, 2) + " detik";

    Serial.println("Blynk Button OFF: Stop recording and stopwatch.");

    // Kirim waktu dengan format numerik ke V7
//    Blynk.virtualWrite(V7, elapsedSeconds);  

    // Jika ingin mengirimkan string waktu dengan satuan, gunakan virtual pin berbeda
    Blynk.virtualWrite(V7, timeWithUnit);  
  }
}


// Function to serve the file download page with a delete button
void handleFilePage() {
  String page = "<html><body>";
  page += "<h1>File Download and Delete</h1>";
  page += "<p>Click the button below to download the file:</p>";
  page += "<a href=\"/download\" target=\"_blank\"><button>Download File</button></a>";
  page += "<br><br>";
  page += "<p>Click the button below to delete the file:</p>";
  page += "<a href=\"/delete\"><button>Delete File</button></a>";
  page += "</body></html>";

  server.send(200, "text/html", page);
}

// Function to handle file download
void handleFileDownload() {
  if (dataFile.open("dataWahyu.csv", O_READ)) {
    server.sendHeader("Content-Type", "text/csv");
    server.sendHeader("Content-Disposition", "attachment; filename=dataWahyu.csv");

    // Stream file content to the client
    String fileContent = "";
    while (dataFile.available()) {
      char ch = dataFile.read();
      fileContent += ch;
    }
    dataFile.close();

    server.send(200, "text/csv", fileContent);
    Serial.println("File served successfully.");
  } else {
    server.send(500, "text/plain", "Failed to open file for download.");
    Serial.println("Error opening dataWahyu.csv for download.");
  }
}

// Function to handle file deletion
void handleFileDelete() {
  if (sd.remove("dataWahyu.csv")) {
    server.send(200, "text/plain", "File deleted successfully.");
    Serial.println("File deleted successfully.");
  } else {
    server.send(500, "text/plain", "Failed to delete the file.");
    Serial.println("Failed to delete the file.");
  }
}

void setup() {
  Serial.begin(115200);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Initialize HTTP server
  server.on("/", HTTP_GET, handleFilePage);      // Home page with buttons
  server.on("/download", HTTP_GET, handleFileDownload); // File download
  server.on("/delete", HTTP_GET, handleFileDelete);   // File delete
  server.begin();                                // Start the server
  Serial.println("HTTP server started.");

    // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // This line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Initialize ADS1115
  ads1.begin(0x48);            // Initialize ADS11115
  ads2.begin(0x49);            // Initialize ADS11115
  ads2.setGain(GAIN_SIXTEEN);

  pinMode(gnd3, INPUT);
  pinMode(gnd4, INPUT);

  // Initialize AC current sensor (SCT-019)
  emon1.current(sct_pin, 108.225);  // Calibration for AC current sensor
    // Initial SD card check in setup
  if (checkSdCard()) {
    Serial.println("SD card is ready.");
    sdReady = true;
  } else {
    Serial.println("SD card not detected or failed to initialize.");
    sdReady = false;
  }

  // Update initial SD card status to Blynk
  updateSdCardStatus();

  delay(10);
}

void loop() {
  currentMillis = millis();

  Blynk.run();
  server.handleClient();  // Handle HTTP server tasks

  // Update readings every 100ms
  if (currentMillis - lastUpdateTime >= 100) {
    // Get sensor readings
    float acVoltage = teganganAc();
    float dcVoltage = teganganDc();
    double acCurrent = arusAc();
    float dcCurrent = arusDc();

    Serial.print("voltage AC");
    Serial.println(acVoltage);
    Serial.print("voltage DC");
    Serial.println(dcVoltage);
    Serial.print("Arus AC");
    Serial.println(acCurrent);
    Serial.print("Arus DC");
    Serial.println(dcCurrent);

    // send to blynk
    Blynk.virtualWrite(V5, acVoltage);
    Blynk.virtualWrite(V1, dcVoltage);
    Blynk.virtualWrite(V2, acCurrent);
    Blynk.virtualWrite(V3, dcCurrent);
    
    // Kirim IP lokal ke Virtual Pin V1
    String localIP = WiFi.localIP().toString();
    Blynk.virtualWrite(V6, localIP);

    // If SD card becomes ready, start recording data
    if (sdReady  && virtualButtonState) {
      if (!headerWritten) {
        writeHeader();
        headerWritten = true;
      }
        saveData(acVoltage, dcVoltage, acCurrent, dcCurrent);  // Call the function to log data to the SD card
        
     } else {
       sdReady = checkSdCard();  // Check if SD card is ready
       headerWritten = false;
     }

    // Update initial SD card status to Blynk
    updateSdCardStatus();
//    readDataFromSD();

    lastUpdateTime = currentMillis;
  }
}

// Function to update SD card status on Blynk
void updateSdCardStatus() {
  if (sdReady) {
    Blynk.virtualWrite(V0, 1);  // SD card is available
  } else {
    Blynk.virtualWrite(V0, 0);  // SD card is not available
  }
}

void writeHeader() {
  if (dataFile.open("dataWahyu.csv", O_CREAT | O_WRITE | O_APPEND)) {
      dataFile.println("---------------------");
      dataFile.println("Timestamp, AC Voltage (V), DC Voltage (V), AC Current (A), DC Current (A), Waktu Tempuh");
      dataFile.println("---------------------");
      dataFile.close();
      Serial.println("Header written.");
  } else {
      Serial.println("Error opening dataWahyu.csv");
  }
}

void saveData(float acVoltage, float dcVoltage, double acCurrent, float dcCurrent) {
  if (dataFile.open("dataWahyu.csv", O_CREAT | O_WRITE | O_APPEND)) {
    DateTime now = rtc.now();
    String timestamp = String(now.year()) + "-" +
                       String(now.month()) + "-" +
                       String(now.day()) + " " +
                       String(now.hour()) + ":" +
                       String(now.minute()) + ":" +
                       String(now.second());

    // Hitung waktu berjalan dalam detik
    unsigned long currentElapsedTime = (millis() - startTime) / 1000;

    // Format data dengan waktu berjalan
    String simpan = timestamp + " , " + String(acVoltage) + " , " + String(dcVoltage) + " , " + 
                    String(acCurrent) + " , " + String(dcCurrent) + " , " + 
                    String(currentElapsedTime) + " detik";

    // Simpan ke file
    dataFile.println(simpan);
    dataFile.close();

    Serial.println("Data saved to SD card with elapsed time.");

  } else {
    Serial.println("Failed to open file on SD card.");
  }
}




float teganganAc() {
  pinMode(gnd3, OUTPUT);
  pinMode(gnd4, OUTPUT);
  digitalWrite(gnd3, LOW);
  digitalWrite(gnd4, LOW);

  float maxCalibrated = 0.0;  // Variable to store the highest calibrated value

  for (int i = 0; i < 10; i++) {
    // Read ADC value and convert to voltage
    float adc = ads2.readADC_Differential_0_1();
    float voltage = 1001 * (adc * 0.007812) / 1000;

    // Apply calibration polynomial
    float calibratedValue = (0.0275 * pow(voltage, 2)) - (0.3552 * voltage) - 26.434;

    // Update maxCalibrated if the current calibratedValue is higher
    if (calibratedValue > maxCalibrated) {
      maxCalibrated = calibratedValue;
    }

    delayMicroseconds(10);  // Delay for sampling
  }

  if (maxCalibrated < 0.00) {
    maxCalibrated = 0.00;
  }

  return maxCalibrated;  // Return the highest calibrated value
}



float teganganDc() {
  ads1.setGain(GAIN_ONE);
  // Read DC voltage using ADS11115
  int16_t adc0 = ads1.readADC_SingleEnded(2);
  voltage = adc0 * 0.125 / 1000;  // Convert to volts
  vbat = voltage * voltageMultiplier;
  vbat = vbat + 2.93;
  vbat = max(vbat, 0.0f);  // Ensure non-negative value
  return vbat;  // Return DC voltage
}

double arusAc() {
  double newIrms = emon1.calcIrms(1500);
  newIrms = max(newIrms, 0.0);  // Ensure non-negative values
  newIrms -= 4; 

  if (newIrms < 0.00){
    newIrms = 0.00;
  }
  return newIrms;
}

float arusDc() {
  ads1.setGain(GAIN_SIXTEEN);
  results = ads1.readADC_Differential_0_1();
  vShunt = results * multiplier;
  arus = vShunt / rShunt ;
//  if ( arus < 0.00 ) {
//    arus = 0.00;
//  }
  return arus;
}
