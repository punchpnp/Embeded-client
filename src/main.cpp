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

// Wi-Fi credentials
const char *ssid = "punchpnp";
const char *password = "0955967996";

// Server details
const char *serverAddress = "172.20.10.3"; // CHANGE TO ESP32#2'S IP ADDRESS
const int serverPort = 80;

WiFiClient TCPclient;

// Ultrasonic sensor pins
const int trigPin = 13; // Trigger pin
const int echoPin = 12; // Echo pin

void setup()
{
  Serial.begin(115200);
  // Set up ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print("loading... ");
  }
  Serial.println();
  Serial.println("Connected to WiFi");

  // Initialize Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);

  // Connect to TCP server (ESP32 #2)
  if (TCPclient.connect(serverAddress, serverPort))
  {
    Serial.println("Connected to TCP server");
  }
  else
  {
    Serial.println("Failed to connect to TCP server");
  }
}

bool ultrasonicEnabled = false; // ULtrasonic Function

void loop()
{
  Blynk.run();

  if (ultrasonicEnabled)
  {
    // Measure distance using ultrasonic sensor
    long duration;
    float distance;

    // Send a 10-microsecond pulse to Trig
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // Read the Echo pin
    duration = pulseIn(echoPin, HIGH);

    // Calculate distance in cm (sound speed = 343 m/s)
    distance = (duration * 0.0343) / 2;

    // Print the distance to the Serial Monitor
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    // Send the distance value to the server
    if (TCPclient.connected())
    {
      // TCPclient.print("Distance: ");
      TCPclient.print(distance);
      // TCPclient.println(" cm");
      TCPclient.print("\n");
      Serial.println("Distance sent to server.");

      // Wait for the server response
      if (TCPclient.available())
      {
        String response = TCPclient.readStringUntil('\n'); 
        Serial.print("Response from server: ");            
        Serial.println(response);                          
      }
    }
    else
    {
      Serial.println("Not connected to server.");
    }
    delay(1000);
  }
  else
  {
    delay(500); 
  }
}

BLYNK_WRITE(V3) // Button for enable/disable ultrasonic
{
  int pinValue = param.asInt();
  ultrasonicEnabled = (pinValue == 1); 
  if (ultrasonicEnabled)
    Serial.println("Ultrasonic function enabled.");
  else
    Serial.println("Ultrasonic function disabled.");
}