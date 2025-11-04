#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
BLECharacteristic *pDateTime = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

// flag to know if we sent the data to the mobile or not
bool sent_to_client = false;

// Variables to put the readings from the sensors
float valueTemp = 0;
float valueHum = 0;
float valueCO = 0;
float valueNO2 = 0;

// TO-DO: test if we can use a String instead of a char[40]
char sensorValues[40];
String date_time;

// UUID of the Prometeo Service
#define PROMETEO_SERVICE_UUID "2c32fd5f-5082-437e-8501-959d23d3d2fb"

// UUID for the sensor characteristic
#define SENSORS_CHARACTERISTIC_UUID "dcaaccb4-c1d1-4bc4-b406-8f6f45df0208"

// UUID for the Date and Time characteristic, we are going to get this value from the mobile
#define DATE_TIME_CHARACTERISTIC_UUID "e39c34e9-d574-47fc-a66e-425cec812aab"

// Needed to know if a device is connected or not (deviceConnected flag)
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) { deviceConnected = true; };

  void onDisconnect(BLEServer *pServer) { deviceConnected = false; }
};

// With this kind of classes y can identify if the client app is writing a characteristic and we can
// take actions. In this case we get the Date and Time from the mobile app
class MyCallbacksDateTime : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0) {
      date_time = "";
      for (int i = 0; i < value.length(); i++) {
        date_time = date_time + value[i];
      }

      Serial.println("*********");
      Serial.print("Date and Time sent by the Prometeo Mobile App: ");
      Serial.println(date_time);
      Serial.println("*********");
    }
  }
};

// Procedure to read the sensor values
void readSensors() {

  // TO-DO: change the random with the readings of the sensors

  valueTemp = random(0, 100);
  valueHum = random(0, 100);
  valueCO = random(0, 100);
  valueNO2 = random(0, 100);
}

// Procedure to format the data before sending to the mobile and storing in the SD Card
void formatDataStream() {

  // TO-DO: adpat to the final format we are going to use
  // We put the sensors values we read before in the characteristic
  // Sprintf with float only works in ESP32, we have to take in account in case we use another
  // processor
  sprintf(sensorValues, "%3.2f %3.2f %3.2f %3.2f", valueTemp, valueHum, valueCO, valueNO2);
}

// Procedure to write the data into the SD Card
void writeSDCard() {

  // TO-DO: write the values into the SD Card. It has to use rotational files
  // We have to add the sent_to_client flag when we write the row in the file to know if we sent or
  // not the data

  Serial.println("Sensor values written to SD Card");
}

// Initial setup
void setup() {

  Serial.println("Initial setup - begin");

  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("Prometeo00001");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(PROMETEO_SERVICE_UUID);

  // Create a BLE Characteristic for the sensors readings
  pCharacteristic = pService->createCharacteristic(SENSORS_CHARACTERISTIC_UUID,
                                                   BLECharacteristic::PROPERTY_READ |
                                                       BLECharacteristic::PROPERTY_NOTIFY |
                                                       BLECharacteristic::PROPERTY_INDICATE);

  // Create a BLE Characteristic for the date and time, it has a PROPERTY_WRITE because we expect
  // the mobile to write this characteristic
  pDateTime = pService->createCharacteristic(DATE_TIME_CHARACTERISTIC_UUID,
                                             BLECharacteristic::PROPERTY_WRITE);

  // Here we indicate the callback function that we'll be called when the Mobile will write the date
  // and time characteristic
  pDateTime->setCallbacks(new MyCallbacksDateTime());

  // Create BLE Descriptors
  pCharacteristic->addDescriptor(new BLE2902());
  pDateTime->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(PROMETEO_SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();

  Serial.println("Waiting a client connection to notify...");

  Serial.println("Initial setup - end");
}

void loop() {

  Serial.println("loop - begin");

  // We read the different sensor values
  readSensors();

  // Takes the diffent values of the sensor and put in the variable sensorValues
  formatDataStream();

  // Flag to know if we sent the data to the mobile, so we can store this flag in the SD Card. In
  // case the row in the file in the SD Card has this value false, we can send later
  sent_to_client = false;

  // In case the Prometeo mobile app connect with the Promete Device, we start notifying

  if (deviceConnected) {

    pCharacteristic->setValue(sensorValues);

    // We send a notification to the client that is connected (our Prometeo mobile app)
    pCharacteristic->notify();

    // We put the flag true to know that we sent this data
    sent_to_client = true;
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                  // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }

  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }

  // Always write in the SD Card, if there is a Prometeo mobile app connected then we can put a flag
  // to now that the data was sent to the mobile (so, we don't have to send again)
  if (sent_to_client)
    Serial.print("SENT TO MOBILE: ");
  else
    Serial.print("NOT SENT TO MOBILE: ");

  Serial.println(sensorValues);

  writeSDCard();

  // With this delay we can control how many times we are going to read the sensor in one minute
  // (the idea is to read once every minute more or less)
  delay(5000);

  Serial.println("loop - end");
}
