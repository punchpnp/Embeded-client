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
const int soilMoistPin = 35;
const int relayPin = 27;

// Water pump variables
unsigned long waterPumpStartTime = 0;        
const unsigned long waterPumpDuration = 10000; 

// Function enable/disable flags
bool ultrasonicEnabled = false; 
bool waterPumpEnabled = false;
bool soilMoistEnabled = true;
bool humidtempEnable = false; 

void setup()
{
  Serial.begin(115200);
  // Set up ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(soilMoistPin, INPUT);
  pinMode(relayPin, OUTPUT);

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

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  if (Blynk.connected())
    Serial.println("Blynk connected!");
  else
    Serial.println("Blynk connection failed!");

  // Connect to TCP server (ESP32 #2)
  if (TCPclient.connect(serverAddress, serverPort))
    Serial.println("Connected to TCP server");
  else
    Serial.println("Failed to connect to TCP server");
}

void soilMoist()
{
  soilMoistEnabled = (100.00 - ((analogRead(soilMoistPin) / 4095.00) * 100.00));

  Serial.print("Soil Moisture: ");
  Serial.print(soilMoistEnabled);
  Serial.println("%");

  Blynk.virtualWrite(V1, soilMoistEnabled);

  if (TCPclient.connected())
  {
    if (soilMoistEnabled < 50)
    {
      TCPclient.print("water");
      Serial.println("\"water\" sent to server.");
    }

    if (TCPclient.available())
    {
      String response = TCPclient.readStringUntil('\n');
      Serial.println(response);

      response.trim();
      if (response == "openPump")
      {
        Serial.print("Response from server: ");
        Serial.println(response);
        Serial.println("\n");
        waterPumpEnabled = true;
      }
    }
  }
  else
  {
    Serial.println("Not connected to server.");
  }
  delay(1000);
}

void waterPump()
{
  if (waterPumpStartTime == 0) // Start the pump if it's not already started
  {
    Serial.println("WaterPump Start");
    digitalWrite(relayPin, HIGH);  // Turn on the water pump
    waterPumpStartTime = millis(); 
  }

  if (millis() - waterPumpStartTime >= waterPumpDuration)
  {
    Serial.println("WaterPump Stop");
    digitalWrite(relayPin, LOW); // Turn off the water pump
    waterPumpEnabled = false;   
    waterPumpStartTime = 0;     
  }
}

void Ultrasonic()
{
  // Measure distance using ultrasonic sensor
  long duration;
  float distance;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = (duration * 0.0343) / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (TCPclient.connected())
  {
    TCPclient.print(distance);
    Serial.println("Distance sent to server.");

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

void loop()
{
  Blynk.run();

  if (ultrasonicEnabled)
    Ultrasonic();
  if (soilMoistEnabled)
    soilMoist();
  if (waterPumpEnabled)
    waterPump();

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

BLYNK_WRITE(V5) // Button for enable/disable Humid and Temperate
{
  int pinValue = param.asInt();
  humidtempEnable = (pinValue == 1);
  if (humidtempEnable)
    Serial.println("Humidity and Temperate function enabled.");
  else
    Serial.println("Humidity and Temperate function disabled.");
}

BLYNK_WRITE(V6) // Button for enable/disable ultrasonic
{
  int pinValue = param.asInt();
  soilMoistEnabled = (pinValue == 1);
  if (soilMoistEnabled)
    Serial.println("Soilmoist function enabled.");
  else
    Serial.println("Soilmoist function disabled.");
}