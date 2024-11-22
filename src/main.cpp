#define BLYNK_TEMPLATE_ID "TMPL6_45WajaT"
#define BLYNK_TEMPLATE_NAME "Project"
#define BLYNK_AUTH_TOKEN "qnQBhPFZ_9isQv_uPwe-Im3U--A2mEOp"

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_I2CDevice.h>
#include "FS.h"
#include <SPI.h>
#include <Adafruit_GFX.h>


const int soilHumidityPin = 33; // Define pin 34 for soil humidity sensor

// Wi-Fi credentials
const char *ssid = "punchpnp";
const char *password = "0955967996";
const char* serverAddress = "172.20.10.3"; // CHANGE TO ESP32#2'S IP ADDRESS
const int serverPort = 80;

WiFiClient TCPclient;

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("loading... ");
  }
  Serial.println();
  Serial.println("Connected to WiFi");

  // Initialize Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);

  // Connect to TCP server (ESP32 #2)
  if (TCPclient.connect(serverAddress, serverPort)) {
    Serial.println("Connected to TCP server");
  } else {
    Serial.println("Failed to connect to TCP server");
  }
}

void loop() {
  Blynk.run();

  int soilHumidityValue = analogRead(soilHumidityPin); // Read the analog value from pin 34
  float soilHumidityPercent = map(soilHumidityValue, 4095, 0, 100, 0); // Convert to percentage

  Blynk.virtualWrite(V4, soilHumidityPercent); // Send soil humidity percentage to Blynk

  Serial.print("Soil Humidity: ");
  Serial.print(soilHumidityValue); // Print the raw soil humidity value to the serial monitor
  Serial.print(" (");
  Serial.print(100 - soilHumidityPercent); // Print the soil humidity percentage to the serial monitor
  Serial.println("%)");

  delay(2000); // Adjust the delay as needed
}

BLYNK_WRITE(V3) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    TCPclient.write('1');
    TCPclient.flush();
    delay(500);
    Serial.println("- Virtual button pressed, sent command: 1");
  } else {
    TCPclient.write('0');
    TCPclient.flush();
    delay(500);
    Serial.println("- Virtual button released, sent command: 0");
  }
}