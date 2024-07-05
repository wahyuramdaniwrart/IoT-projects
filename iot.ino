#define BLYNK_TEMPLATE_ID "TMPL6h30rWgrd"
#define BLYNK_TEMPLATE_NAME "Sensor Ketinggian Air Pipa Hidroponik"
#define BLYNK_AUTH_TOKEN "RZgPWFo0HE8SiyKbdeoWe0yIkq-7_AkE"
/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
/* Fill in information from Blynk Device Info here */
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// Your WiFi credentials.
char ssid[] = "LAB INKUBATOR BISNIS FAPERTA";
char pass[] = "ummat9008";

//////////////////////////////////////////
#define triggerPin D8
#define echoPin D7
long duration, jarak;

//////////////////////////////////////////
#define relay D4

///////////////////////////////
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>  
LiquidCrystal_I2C lcd(0x27, 16, 2);

unsigned long interval;


//////////////////////////////

BLYNK_WRITE(V1) {
  digitalWrite(D4, param.asInt());
}

void setup() {
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  lcd.init();
  lcd.clear();         
  lcd.backlight();
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
   pinMode(relay, OUTPUT);
}

void loop() {

  if (millis() - interval > 1000) {
    
   hcrs04();
   pompa();
//   LCD();
  
  interval = millis();
  }

  Blynk.run();
}

void hcrs04() {
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2); 
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10); 
  digitalWrite(triggerPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  jarak = (duration / 2) / 29.1;
  Serial.println("jarak :");
  Serial.print(jarak);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("TINGGI : " + String(jarak) + " Cm");
  Serial.println(" cm");

   Blynk.virtualWrite(V0, jarak);
  // yield();  // Menghindari watchdog reset
}

void pompa() {
  int relayState = digitalRead(relay);
  
  // Jika relay high (aktif)
  if (relayState == HIGH) {
    lcd.setCursor(4, 0);
    lcd.print("POMPA ON ");
  } 
  // Jika relay low (mati)
  else {
    lcd.setCursor(4, 0);
    lcd.print("POMPA OFF");
  }
  // yield();  // Menghindari watchdog reset
}
