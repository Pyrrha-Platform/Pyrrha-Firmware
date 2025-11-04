#include "Adafruit_NeoPixel.h"
#include "BLE2902.h"
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "DHT.h"
#include "SD.h"
#include "SPI.h"

// BLE Server
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic_TEMP = NULL;
BLECharacteristic *pCharacteristic_HUM = NULL;
BLECharacteristic *pCharacteristic_CO = NULL;
BLECharacteristic *pCharacteristic_NO2 = NULL;

// BLE Server
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t preHeatSeconds = 10;
float tempValue = 0;
float humValue = 0;
float coValue = 0;
float no2Value = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define PROMETEO_SERVICE_UUID "2c32fd5f-5082-437e-8501-959d23d3d2fb"
#define TEMP_CHARACTERISTIC_UUID "dcaaccb4-c1d1-4bc4-b406-8f6f45df0208"
#define HUM_CHARACTERISTIC_UUID "e39c34e9-d574-47fc-a66e-425cec812aab"
#define CO_CHARACTERISTIC_UUID "a62e48ca-0599-4757-9c24-b76a9b970ef1"
#define NO2_CHARACTERISTIC_UUID "e1e1687a-65ab-4c7e-a8ca-cab9bb1b52c2"

#define DHTPIN 27     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

#define LED_PIN A0 //(ESP32 could be pin #26) //mine:5
#define LED_COUNT 1
#define BRIGHTNESS 50
Adafruit_NeoPixel statusLED(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

#define PRE_PIN                                                                                    \
  14 // Replace DigitalPin# by the number where the Pre connection of the CJMCU-4514 is connected
#define VNOX_PIN 36 // Replace A3 by the AnalogPin# you are using
#define VRED_PIN 39 // Replace A4 by the AnalogPin# you are using

File myFile;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) { deviceConnected = true; };
  void onDisconnect(BLEServer *pServer) { deviceConnected = false; };
};

void setup() {
  Serial.begin(115200);
  Serial.println("Beginning setup() here");

  // TODO: get timestamp from cloud / connected mobile device

  // TODO: get name from cloud / connected mobile device
  BLEDevice::init("Prometeo00001");    // Create the BLE Device
  pServer = BLEDevice::createServer(); // Create the BLE Server
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(PROMETEO_SERVICE_UUID); // Create the BLE Service

  // Create a BLE Characteristics
  pCharacteristic_TEMP = pService->createCharacteristic(TEMP_CHARACTERISTIC_UUID,
                                                        BLECharacteristic::PROPERTY_READ |
                                                            BLECharacteristic::PROPERTY_NOTIFY |
                                                            BLECharacteristic::PROPERTY_INDICATE);
  pCharacteristic_HUM = pService->createCharacteristic(HUM_CHARACTERISTIC_UUID,
                                                       BLECharacteristic::PROPERTY_READ |
                                                           BLECharacteristic::PROPERTY_NOTIFY |
                                                           BLECharacteristic::PROPERTY_INDICATE);
  pCharacteristic_CO = pService->createCharacteristic(CO_CHARACTERISTIC_UUID,
                                                      BLECharacteristic::PROPERTY_READ |
                                                          BLECharacteristic::PROPERTY_NOTIFY |
                                                          BLECharacteristic::PROPERTY_INDICATE);
  pCharacteristic_NO2 = pService->createCharacteristic(NO2_CHARACTERISTIC_UUID,
                                                       BLECharacteristic::PROPERTY_READ |
                                                           BLECharacteristic::PROPERTY_NOTIFY |
                                                           BLECharacteristic::PROPERTY_INDICATE);

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic_TEMP->addDescriptor(new BLE2902());
  pCharacteristic_HUM->addDescriptor(new BLE2902());
  pCharacteristic_CO->addDescriptor(new BLE2902());
  pCharacteristic_NO2->addDescriptor(new BLE2902());

  pService->start(); // Start the service

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(PROMETEO_SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");

  // Initialize SD card...
  Serial.println("Initializing SD card...");
  if (!SD.begin(4)) // 4 in some setups, 33 in others
  {
    Serial.println("SD card initialization failed!");
    while (1)
      ;
  }
  Serial.println("SD card initialized.");
  // TODO: Name file according to firefighterID and timestamp

  // Turn on LED
  // TODO: Add connectivity status LED
  Serial.println("Turning on LED");
  statusLED.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  statusLED.setPixelColor(0, statusLED.Color(0, 0, 255)); // Blue
  statusLED.show();                                       // Turn OFF all pixels ASAP
  statusLED.setBrightness(50);                            // Set BRIGHTNESS to about 1/5 (max = 255)

  // Turn on dht22 temp/hum sensor
  dht.begin();

  // set up MiCS-4514 gas sensor
  pinMode(PRE_PIN, OUTPUT); // set preheat pin

  Serial.println("Finished setup() here");
  Serial.println("");

} // end setup()

void loop() {

  // Preheat gas sensor
  Serial.println("Preheating gas sensor...");
  digitalWrite(PRE_PIN, 1);

  // Average temp and humidity values for 10 seconds
  float sumTemp = 0;
  float sumHum = 0;
  uint32_t count = 0;
  // uint32_t preHeatTime = preHeatSeconds*1000;
  while (count < preHeatSeconds) {
    sumTemp = sumTemp + dht.readTemperature(); // temp in Celsius; for temp in Fahrenheit, use
                                               // dht.readTemperature(true);
    sumHum = sumHum + dht.readHumidity();
    delay(1000);
    count++;
  }
  tempValue = sumTemp / count;
  humValue = sumHum / count;

  Serial.print("Avg Temperature: ");
  Serial.print(tempValue);

  Serial.print(" # Avg Humidity: ");
  Serial.println(humValue);

  // Average gas sensor values for 10 seconds
  digitalWrite(PRE_PIN, 0);
  Serial.println("Done with preheat. Measuring gases...");

  float sumCO = 0;
  float sumNO2 = 0;
  uint32_t gasCount = 0;
  while (gasCount < 10) {
    sumCO = sumCO + analogRead(VRED_PIN);
    sumNO2 = sumNO2 + analogRead(VNOX_PIN);
    delay(1000);
    gasCount++;
  }
  coValue = sumCO / gasCount;
  no2Value = sumNO2 / gasCount;

  Serial.print("Avg Vco: ");
  Serial.print(coValue, DEC);

  Serial.print(" # Avg Vno2: ");
  Serial.println(no2Value, DEC);

  // Build the payload to send to SD card
  String payload = "Temperature: ";
  payload += tempValue;
  payload += "C, Humidity: ";
  payload += humValue;
  payload += "%, CO V: ";
  payload += coValue;
  payload += ", NO2 V: ";
  payload += no2Value;
  payload += " # ";

  // Write characteristic values to SD card
  Serial.println("Writing payload to SD card...");
  Serial.println(payload);
  // call SD card write function here
  myFile = SD.open("/test.txt", FILE_WRITE);
  if (myFile) {
    myFile.println(payload);
    myFile.close();
    Serial.println("SD card write successful");
  } else {
    Serial.println("SD card write failed");
  }

  // Notify device of new values
  if (deviceConnected) {

    Serial.println("Transmitting characteristics...");
    uint32_t btDelay = 3; // bluetooth stack will go into congestion, if too many packets are sent,
                          // in 6 hours test i was able to go as low as 3ms

    notify_value(tempValue, pCharacteristic_TEMP); // Send Temperature to the BLE Client
    delay(btDelay);
    notify_value(humValue, pCharacteristic_HUM); // Send Humidity to the BLE Client
    delay(btDelay);
    notify_value(coValue, pCharacteristic_CO); // Send CO to the BLE Client
    delay(btDelay);
    notify_value(no2Value, pCharacteristic_NO2); // Send NO2 to the BLE Client
    delay(btDelay);

  } // end if(deviceConnected)
  // TODO: not sure if this part needs updating/what it's trying to do
  //  Disconnecting
  else if (!deviceConnected && oldDeviceConnected) {
    delay(500);                  // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // Connecting
  else if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }

  // TODO: update LED color based on thresholds
  updateStatusLED(no2Value);

  // some programmable delay between measurements - for now 30s
  Serial.println("Characteristics sent. Delaying 30 seconds...");
  Serial.println("");
  delay(30000);

} // end loop()

void notify_value(int value, BLECharacteristic *pCharacteristic) {
  pCharacteristic->setValue((uint8_t *)&value, 4);
  pCharacteristic->notify();
}

void updateStatusLED(float no2) {
  // TODO: have this actually based on thresholds for each value / machine learning data
  // no2 voltage > 1700 is just for testing
  if (no2 > 1720) {
    statusLED.setPixelColor(0, statusLED.Color(255, 0, 0)); // Green
    statusLED.show();
  } else {
    statusLED.setPixelColor(0, statusLED.Color(0, 255, 0)); // Red
    statusLED.show();
  }
}
