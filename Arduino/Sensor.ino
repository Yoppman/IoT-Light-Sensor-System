#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "WiFiCredentials.h" // WiFi Credentials

// UDP Setup
WiFiUDP udp;
unsigned int localUdpPort = 8266;  
char incomingPacket[255];  // Buffer 

// Variables for light sensor data and timing
const int lightSensorPin = A0;
int sensorValues[5];  // Store the last 5 seconds  data
int currentIndex = 0;
unsigned long previousMillisSensor = 0;
unsigned long previousMillisLED = 0;
unsigned long previousMillisUDP = 0;
bool communicationStarted = false;
bool startSending = false;

// LED Blink Interval and Light Sensor Interval
const long ledInterval = 500;  // 0.5 second for LED blinking
const long sensorInterval = 1000;  // 1 second for collecting light sensor data
const long udpInterval = 2000;  // 2 seconds for sending data via UDP

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // Start listening for UDP message
  udp.begin(localUdpPort);
  Serial.printf("Listening on UDP port %d\n", localUdpPort);
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(incomingPacket, 255);
    if (len > 0) {
      incomingPacket[len] = '\0';
    }
    Serial.printf("Received packet: %s\n", incomingPacket);
    if (String(incomingPacket).indexOf("Start Communication") != -1) {
      Serial.println("Starting communication...");
      communicationStarted = true;
      currentIndex = 0;
      digitalWrite(LED_BUILTIN, LOW);  // Turn on the ESP8266 onboard LED
    }
    else if (String(incomingPacket).indexOf("Stop Communication") != -1) {
      Serial.println("Stop communication...");
      communicationStarted = false;
      startSending = false;
      digitalWrite(LED_BUILTIN, HIGH);  // Turn off the ESP8266 onboard LED
    }
  }

  if (communicationStarted) {
    unsigned long currentMillis = millis();

    // Blink LED every 0.5 seconds
    if (currentMillis - previousMillisLED >= ledInterval) {
      previousMillisLED = currentMillis;
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }

    // Collect light sensor values every 1 second
    if (currentMillis - previousMillisSensor >= sensorInterval) {
      previousMillisSensor = currentMillis;

      // Read sensor value
      int sensorValue = analogRead(lightSensorPin);
      sensorValues[currentIndex] = sensorValue;
      currentIndex = (currentIndex + 1) % 5;  
      Serial.printf("Light sensor value: %d\n", sensorValue);

      // If Index is 0 again, then we must have collected 5 second worth of data, and then start sending
      if (currentIndex == 0) {
        startSending = true;
      }
    }

    // Send the average sensor value every 2 seconds
    if (startSending && currentMillis - previousMillisUDP >= udpInterval) {
      previousMillisUDP = currentMillis;

      int sum = 0;
      for (int i = 0; i < 5; i++) {
        sum += sensorValues[i];
      }
      int averageValue = sum / 5;

      // Send the average value
      udp.beginPacket(udp.remoteIP(), udp.remotePort());
      udp.printf("Average light sensor value: %d", averageValue);
      udp.endPacket();
      Serial.println("Sent average light sensor value via UDP.");
    }
  }
}