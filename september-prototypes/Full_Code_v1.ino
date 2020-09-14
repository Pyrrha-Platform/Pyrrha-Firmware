
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "DHT.h"
#include "SD.h"
#include "SPI.h"
#include "Adafruit_NeoPixel.h"


// Bluetooth Low Energy configuration
//BLE Server
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
BLECharacteristic *pDateTime = NULL;

//BLE Server
bool deviceConnected = false;
bool oldDeviceConnected = false;

// flag to know if we sent the data to the mobile or not
bool sent_to_client = false;


// See the following for generating UUIDs:https://www.uuidgenerator.net/
// BLE: UUID of the Prometeo Service
#define PROMETEO_SERVICE_UUID "2c32fd5f-5082-437e-8501-959d23d3d2fb"

// BLE: UUID for sensors characteristic(all sensors readings in only one characteristic)
#define SENSORS_CHARACTERISTIC_UUID "dcaaccb4-c1d1-4bc4-b406-8f6f45df0208"

// BLE: UUID for the Date and Time characteristic, we are going to get this value from the mobile
#define DATE_TIME_CHARACTERISTIC_UUID "e39c34e9-d574-47fc-a66e-425cec812aab"

// Global Variables definition
// Variables to put the readings from the sensors
uint32_t preHeatSeconds = 30;
float tempValue = 0;
float humValue = 0;
float coValue = 0;
float no2Value = 0;
uint32_t gasCount = 0;

// sensorValues is the variable where we put readings from all sensors
char sensorValues[40];
String date_time;

//  DHT22 configuration 
#define DHTPIN 27     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   
DHT dht(DHTPIN, DHTTYPE);

//  LED(status) configuration 
#define LED_PIN    A0 //(ESP32 could be pin #26) 
#define LED_COUNT  1 
#define BRIGHTNESS 50
Adafruit_NeoPixel statusLED(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

//  SD CARD ChipSelect PIN configuration 
#define CS_PIN   33 //4 in some setups, 33 in others

// CJMCU-4541 configuration 
#define PRE_PIN          14 // Replace DigitalPin# by the number where the Pre connection of the CJMCU-4514 is connected
#define VNOX_PIN         36 // Replace A3 by the AnalogPin# you are using
#define VRED_PIN         39 // Replace A4 by the AnalogPin# you are using



//  Functions Definition 

// Needed to know if a device is connected or not (deviceConnected flag)
class MyServerCallbacks: public BLEServerCallbacks {
      void onConnect(BLEServer* pServer) {
           deviceConnected = true;
      };

      void onDisconnect(BLEServer* pServer) {
          deviceConnected = false;
      }
  
};


// With this kind of classes we can identify if the client app is writing a characteristic and we can take actions. 
//In this case we get the Date and Time from the mobile app
class MyCallbacksDateTime: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {
        date_time = "";
        for (int i = 0; i < value.length(); i++){
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
//To-DO: Add function related to 10min and 8hours Thresholds to change Led status color in case no internet connection between cellphone and IBM IoT Platform
void readSensors() {

 //Preheat gas sensor
  Serial.println("Preheating gas sensor..."); //lo puse en el setup()
  digitalWrite(PRE_PIN, 1);
  
  //Average temp and humidity values for 10 seconds
  float sumTemp = 0;
  float sumHum = 0;
  uint32_t count = 0;
  //uint32_t preHeatTime = preHeatSeconds*1000;
  while(count < preHeatSeconds)
  {
     sumTemp = sumTemp + dht.readTemperature(); //temp in Celsius; for temp in Fahrenheit, use dht.readTemperature(true);
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

  
  //Done with Preheat
  digitalWrite(PRE_PIN, 0);
  Serial.println("Done with preheat. Measuring gases...");

 //Average gas sensor values for 10 seconds
  float sumCO = 0; //*** pasa a ser variable global
  float sumNO2 = 0;
  uint32_t gasCount = 0;
  
     while (gasCount < preHeatSeconds)
  {
    sumCO = sumCO + analogRead(VRED_PIN);
    sumNO2 = sumNO2 + analogRead(VNOX_PIN);
    delay(1000);
    gasCount++;
  }
  coValue = sumCO / gasCount;
  no2Value = sumNO2 / gasCount;
  Serial.print("Avg Vco: ");
  Serial.print(coValue); 

  Serial.print(" # Avg Vno2: ");
  Serial.println(no2Value);
  // Sprintf with float only works in ESP32, we have to take in account in case we use another processor
  sprintf(sensorValues, "%3.2f %3.2f %3.2f %3.2f", tempValue , humValue, coValue, no2Value);
  
}



// Procedure to write the data into the SD Card 
// TO-DO: write the values into the SD Card identified by date_time of the first reading. It has to use rotational files 
// We have to add the sent_to_client flag when we write the row in the file to know if we sent or not the data

void writeSDCard(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.println(message)){
       
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
    Serial.println("SD card write successful");
}

//TODO: modify this function: 1) it should work  with thresholds from machine learning data; and 2)for no-internet connection: Prometeo
 //device should be able to update LED status based on 10 min threshold and/or 8hours AEGLs threshold per gases.
 
  void updateStatusLED(float no2) 
 {
  //TODO: have this actually based on thresholds for each value / machine learning data 
  //no2 voltage > 1700 is just for testing
  if ( no2 > 1720)
  {
    statusLED.setPixelColor(0, statusLED.Color(255, 0, 0)); // Green
    statusLED.show();
    
  }
  else
  {
    statusLED.setPixelColor(0, statusLED.Color(0, 255, 0)); // Red
    statusLED.show();
  }
} 

//initial setup

void setup(){
    Serial.println("Initial setup - begin");
    Serial.begin(115200);
    
     // Turn on dht22 temp/hum sensor
    dht.begin();

     //Preheat dht sensor
    Serial.println("Preheating gas sensor...");
    digitalWrite(PRE_PIN, 1);
    
    // Create the BLE Device
    BLEDevice::init("Prometeo00001");
    
    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    // Create the BLE Service
    BLEService *pService = pServer->createService(PROMETEO_SERVICE_UUID);
    
    // Create a BLE Characteristic for the sensors readings
    pCharacteristic = pService->createCharacteristic(
                          SENSORS_CHARACTERISTIC_UUID,
                          BLECharacteristic::PROPERTY_READ|
                          BLECharacteristic::PROPERTY_NOTIFY |
                          BLECharacteristic::PROPERTY_INDICATE
                          );
       
    
    // Create a BLE Characteristic for the date and time, it has a PROPERTY_WRITE because we expect the mobile to write this characteristic
    pDateTime = pService->createCharacteristic(
                          DATE_TIME_CHARACTERISTIC_UUID,
                          BLECharacteristic::PROPERTY_WRITE
                        );
    
    // Here we indicate the callback function that we'll be called when the Mobile will write the date and time characteristic
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
    pAdvertising->setMinPreferred(0x0);     // set value to 0x00 to not advertise this parameter
    BLEDevice::startAdvertising();
    
    Serial.println("Waiting a client connection to notify...");


     // Initialize SD card...
  Serial.println("Initializing SD card...");
  if(!SD.begin(CS_PIN)) // 
  {
    Serial.println("SD card initialization failed!");
    while (1);
  }
  Serial.println("SD card initialized.");
  // TODO: Name file according to firefighterID and timestamp

  uint8_t cardType = SD.cardType();
     delay(5000);
    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }
    
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
    delay(5000);
    
    Serial.printf("Total space of SD Card: %lluMB\n", SD.totalBytes() / (1024 * 1024));
    Serial.printf("Used space in SD Card: %lluMB\n", SD.usedBytes() / (1024 * 1024));


  // Turn on LED 
  // TODO: Add connectivity status LED
  Serial.println("Turning on LED");
  statusLED.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  statusLED.setPixelColor(0, statusLED.Color(0, 0, 255)); // Blue
  statusLED.show();            // Turn OFF all pixels ASAP
  statusLED.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)

    
    // Turn on dht22 temp/hum sensor
  dht.begin();

  // set up MiCS-4514 gas sensor
  pinMode(PRE_PIN, OUTPUT); //set preheat pin
 
  Serial.println("Finished initial setup() here");
  Serial.println("");
  
}// end setup()


void loop(){
    Serial.println("loop - begin");

    // We read the different sensor values
    readSensors();
     

    // Flag to know if we sent the data to the mobile, so we can store this flag in the SD Card. In case the row in the file in the SD Card has this value false, we can send later
    sent_to_client = false;
    

    if (deviceConnected) {
        
        pCharacteristic->setValue(sensorValues);

        // We send a notification to the client that is connected (our Prometeo mobile app)
        pCharacteristic->notify();

        // We put the flag true to know that we sent this data
        sent_to_client = true;
        writeSDCard(SD, "/sensorsReadingData.txt", date_time.c_str()); //Writing date and time in SD card before sensorReading DataLog
    }
    
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected; 
    }

    // Always write in the SD Card, if there is a Prometeo mobile app connected then we can put a flag to now that the data was sent to the mobile (so, we don't have to send again)
    if (sent_to_client)
        Serial.print("SENT TO MOBILE: ");
    else 
        Serial.print("NOT SENT TO MOBILE: ");

    Serial.println(sensorValues);

    writeSDCard(SD, "/sensorsReadingData.txt", sensorValues);

     //TODO: update LED color based on thresholds
   updateStatusLED(no2Value);
   //some programmable delay between measurements - for now 30s
   Serial.println("Characteristics sent. Delaying 30 seconds...");
   delay(30000);
   Serial.println("loop - end");
   Serial.println(" ");
    
    
}//end loop
