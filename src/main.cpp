// BLYNK
#define BLYNK_TEMPLATE_ID "TMPL6_45WajaT"
#define BLYNK_TEMPLATE_NAME "Project"
#define BLYNK_AUTH_TOKEN "qnQBhPFZ_9isQv_uPwe-Im3U--A2mEOp"

// FIREBASE
#define FIREBASE_API_KEY "AIzaSyDlku4rxvpDzrvbXsaa_PK__VbLUtF4GKY"
#define FIREBASE_DATABASE_URL "https://embreddedproject-default-rtdb.asia-southeast1.firebasedatabase.app/"

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_I2CDevice.h>
#include "FS.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// Wi-Fi credentials
const char *ssid = "GAMEROOM_2.4G";
const char *password = "66035r00mgam3";

// Server details
const char *serverAddress = "172.20.10.3"; // CHANGE TO ESP32#2'S IP ADDRESS
const int serverPort = 80;

bool FB_signupOK = false;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

WiFiClient TCPclient;

// Ultrasonic sensor pins
const int trigPin = 13; // Trigger pin
const int echoPin = 12; // Echo pin
const int soilMoistPin = 35;
const int relayPin = 27;

unsigned long previousMillis = 0;
const unsigned long interval = 2000;

// Water pump variables
unsigned long waterPumpStartTime = 0;
const unsigned long waterPumpDuration = 10000;

// Function enable/disable flags
bool ultrasonicEnabled = false;
bool waterPumpEnabled = false;
bool soilMoistEnabled = false;
bool humidtempEnabled = false;
bool lightSensorEnabled = true;

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

  // Initialize Firebase
  config.api_key = FIREBASE_API_KEY;
  config.database_url = FIREBASE_DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("Firebase sign up OK!");
    FB_signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
    Serial.println("Firebase sign up failed!");
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Connect to TCP server (ESP32 #2)
  if (TCPclient.connect(serverAddress, serverPort))
    Serial.println("Connected to TCP server");
  else
    Serial.println("Failed to connect to TCP server");
}

void handleFirebaseStoreData(String _path, String _data)
{
  if (Firebase.ready() && FB_signupOK)
  {
    if (Firebase.RTDB.setString(&fbdo, _path, _data))
    {
      Serial.println();
      Serial.print(_data);
      Serial.print(" - Successfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
    }
    else
    {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
  }
}

void soilMoist()
{
  int soilMoistValue = (100.00 - ((analogRead(soilMoistPin) / 4095.00) * 100.00));
  Serial.print("Soil Moisture: ");
  Serial.print(soilMoistValue);
  Serial.println("%");

  Blynk.virtualWrite(V1, soilMoistValue);
  handleFirebaseStoreData("Client/SoilMoist", soilMoistValue.toString());

  if (TCPclient.connected())
  {
    if (soilMoistValue < 50)
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
}

void waterPump()
{
  if (waterPumpStartTime == 0) // Start the pump if it's not already started
  {
    Serial.println("WaterPump Start");
    digitalWrite(relayPin, HIGH); // Turn on the water pump
    waterPumpStartTime = millis();
    handleFirebaseStoreData("Client/WaterPump", "on");
  }

  if (millis() - waterPumpStartTime >= waterPumpDuration)
  {
    Serial.println("WaterPump Stop");
    digitalWrite(relayPin, LOW); // Turn off the water pump
    waterPumpEnabled = false;
    waterPumpStartTime = 0;
    handleFirebaseStoreData("Client/WaterPump", "off");
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

  handleFirebaseStoreData("Client/Ultrasonic", distance.toString());

  if (TCPclient.connected())
  {
    TCPclient.print(distance);
    Serial.println("Distance sent to server.");

    if (TCPclient.available())
    {
      String response = TCPclient.readStringUntil('\n');
      response.trim();
      
      Serial.print("Response from server: ");
      Serial.println(response);
    }
  }
  else
  {
    Serial.println("Not connected to server.");
  }
}

void lightSensor()
{
  if (TCPclient.connected())
  {
    if (TCPclient.available())
    {
      String response = TCPclient.readStringUntil('\n');
      response.trim();
      Serial.print("Response Light Status from server: ");
      Serial.println(response);
    }
  }
  else
  {
    Serial.println("Not connected to server.");
  }
}

void loop()
{
  Blynk.run();

  if ((millis() - previousMillis > 2000 || previousMillis == 0))
  {
    previousMillis = millis();
    if (ultrasonicEnabled)
      Ultrasonic();
    if (soilMoistEnabled)
      soilMoist();
    if (waterPumpEnabled)
      waterPump();
    if (lightSensorEnabled)
      lightSensor();
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

BLYNK_WRITE(V5) // Button for enable/disable Humid and Temperate
{
  int pinValue = param.asInt();
  humidtempEnabled = (pinValue == 1);
  if (humidtempEnabled)
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