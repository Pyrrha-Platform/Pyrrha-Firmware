#include <DHT.h>
// This file MUST be in the same folder as the .ino file.
#include "config.h"
// Setting up the DHT Sensor

// #define DHTPIN 4
// #define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  Serial.println();

  // Initialize temperature/humidity sensor
  dht.begin();
}
void loop() {
  // Build the payload to send to the IBM IoT Platform
  String payload = "{\"Temperature\":";
  payload += dht.readTemperature();
  payload += ", \"Humidity\":";
  payload += dht.readHumidity();
  // payload += ", \"CO\":";
  // payload += mq7.getPPM();
  payload += "}";

  Serial.print("Sending payload: ");
  Serial.println(payload);

  // Give time to send the whole request
  delay(5000);
  Serial.println("");

  // Activate deep sleep mode - it will remain in sleep mode for the number of seconds that we setup
  // in the esp_sleep_enable_timer_wakeup function. Not compatible with iOS devices - comment out
  // the following line to enable compatibility with iOS devices. esp_deep_sleep_start();
}
