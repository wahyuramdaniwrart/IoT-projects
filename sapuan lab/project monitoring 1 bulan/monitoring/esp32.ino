#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#define BLYNK_TEMPLATE_ID "TMPL6hntJtddh"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "DlhXkzCYdTqU0yy38MGArcvqOB514naJ"

// Inisialisasi variabel untuk Blynk
char auth[] = BLYNK_AUTH_TOKEN; // Auth Token Blynk Anda
char ssid[] = "BABY POCO";      // SSID WiFi Anda
char pass[] = "ramdaniwahyu";   // Password WiFi Anda

// Inisialisasi variabel untuk menyimpan nilai sensor
float sensorValues[8] = {0};

// Buffer untuk menyimpan data serial
String serialBuffer = "";

void setup()
{
    Serial.begin(115200);
    Blynk.begin(auth, ssid, pass);

    // Inisialisasi pin Virtual Blynk (misalnya V0 hingga V7)
    for (int i = 0; i < 8; i++)
    {
        Blynk.virtualWrite(V0 + i, sensorValues[i]);
    }
}

void loop()
{
    Blynk.run();

    // Baca data dari Serial
    while (Serial.available() > 0)
    {
        char inChar = (char)Serial.read();
        serialBuffer += inChar;

        // Jika menemukan tanda akhir '>', proses data
        if (inChar == '>')
        {
            // Temukan posisi tanda mulai '<'
            int startIdx = serialBuffer.indexOf('<');
            if (startIdx >= 0)
            {
                // Ekstrak data antara tanda mulai '<' dan tanda akhir '>'
                String data = serialBuffer.substring(startIdx + 1, serialBuffer.length() - 1);
                Serial.println(data);

                // Pecah data menjadi 8 nilai
                int index = 0;
                int startIndex = 0;
                for (int i = 0; i < data.length(); i++)
                {
                    if (data[i] == ',')
                    {
                        sensorValues[index] = data.substring(startIndex, i).toFloat();
                        startIndex = i + 1;
                        index++;
                    }
                }
                // Tangani nilai terakhir
                if (index < 8)
                {
                    sensorValues[index] = data.substring(startIndex).toFloat();
                }

                // Kirim data sensor ke Blynk
                for (int i = 0; i < 8; i++)
                {
                    Blynk.virtualWrite(V0 + i, sensorValues[i]);
                }
            }

            // Bersihkan buffer
            serialBuffer = "";
        }
    }

    delay(100); // Tambahkan jeda 100 milidetik
}
