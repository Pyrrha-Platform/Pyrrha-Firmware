#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic_TEMP = NULL;
BLECharacteristic *pCharacteristic_HUM = NULL;
BLECharacteristic *pCharacteristic_CO = NULL;
BLECharacteristic *pCharacteristic_NO2 = NULL;
BLECharacteristic *pCharacteristic_FOR = NULL;
BLECharacteristic *pCharacteristic_BEN = NULL;
BLECharacteristic *pCharacteristic_ACR = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define PROMETEO_SERVICE_UUID "2c32fd5f-5082-437e-8501-959d23d3d2fb"
#define TEMP_CHARACTERISTIC_UUID "dcaaccb4-c1d1-4bc4-b406-8f6f45df0208"
#define HUM_CHARACTERISTIC_UUID "e39c34e9-d574-47fc-a66e-425cec812aab"
#define CO_CHARACTERISTIC_UUID "a62e48ca-0599-4757-9c24-b76a9b970ef1"
#define NO2_CHARACTERISTIC_UUID "e1e1687a-65ab-4c7e-a8ca-cab9bb1b52c2"
#define FOR_CHARACTERISTIC_UUID "046c65a4-4a4d-4e18-8b72-77e9b62521c9"
#define BEN_CHARACTERISTIC_UUID "b973c55d-4932-4e51-8bc4-158bf3b57071"
#define ACR_CHARACTERISTIC_UUID "8c232843-fcb9-49c2-b453-13adefad921a"

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
  }
};

void setup()
{
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("Prometeo00001");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(PROMETEO_SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic_TEMP = pService->createCharacteristic(
      TEMP_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);
  pCharacteristic_HUM = pService->createCharacteristic(
      HUM_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  pCharacteristic_CO = pService->createCharacteristic(
      CO_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  pCharacteristic_NO2 = pService->createCharacteristic(
      NO2_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  pCharacteristic_FOR = pService->createCharacteristic(
      FOR_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  pCharacteristic_BEN = pService->createCharacteristic(
      BEN_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  pCharacteristic_ACR = pService->createCharacteristic(
      ACR_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);
  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic_TEMP->addDescriptor(new BLE2902());
  pCharacteristic_HUM->addDescriptor(new BLE2902());
  pCharacteristic_CO->addDescriptor(new BLE2902());
  pCharacteristic_NO2->addDescriptor(new BLE2902());
  pCharacteristic_FOR->addDescriptor(new BLE2902());
  pCharacteristic_BEN->addDescriptor(new BLE2902());
  pCharacteristic_ACR->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(PROMETEO_SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void notify_value(int value, BLECharacteristic *pCharacteristic)
{
  pCharacteristic->setValue((uint8_t *)&value, 4);
  pCharacteristic->notify();
}

void loop()
{
  // notify changed value
  if (deviceConnected)
  {

    // Send Temperature to the BLE Client
    value = random(0, 100);
    notify_value(value, pCharacteristic_TEMP);
    Serial.print("Temperature: ");
    Serial.print(value);
    delay(3); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms

    // Send Humidity to the BLE Client
    value = random(0, 100);
    notify_value(value, pCharacteristic_HUM);
    Serial.print(" # Humidity: ");
    Serial.print(value);
    delay(3); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms

    // Send CO to the BLE Client
    value = random(0, 100);
    notify_value(value, pCharacteristic_CO);
    Serial.print(" # CO: ");
    Serial.print(value);
    delay(3); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms

    // Send NO2 to the BLE Client
    value = random(0, 100);
    notify_value(value, pCharacteristic_NO2);
    Serial.print(" # NO2: ");
    Serial.print(value);
    delay(3); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms

    // Send Formaldehyde to the BLE Client
    value = random(0, 100);
    notify_value(value, pCharacteristic_FOR);
    Serial.print(" # Formaldehyde: ");
    Serial.print(value);
    delay(3); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms

    // Send Benzene to the BLE Client
    value = random(0, 100);
    notify_value(value, pCharacteristic_BEN);
    Serial.print(" # Benzene: ");
    Serial.print(value);
    delay(3); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms

    // Send Acroleine to the BLE Client
    value = random(0, 100);
    notify_value(value, pCharacteristic_ACR);
    Serial.print(" # Acroleine: ");
    Serial.println(value);
    delay(300); // We wait 300 ms to update
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected)
  {
    delay(500);                  // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected)
  {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}
