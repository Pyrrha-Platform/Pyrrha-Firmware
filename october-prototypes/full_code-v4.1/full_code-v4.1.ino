
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "DHT.h"
#include "SD.h"
#include "SPI.h"
#include "Adafruit_NeoPixel.h"
#include <WiFi.h>  //added to get MAC Address
#include <esp_wifi.h>

String prometeoID;

// Bluetooth Low Energy configuration
BLEServer *pServer = NULL; 
BLECharacteristic *pCharacteristic = NULL;
BLECharacteristic *pDateTime = NULL;
BLECharacteristic *pStatusCloud = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool sent_to_client = false; // flag to know if we sent the data to the mobile or not
// See the following for generating UUIDs:https://www.uuidgenerator.net/
#define PROMETEO_SERVICE_UUID "2c32fd5f-5082-437e-8501-959d23d3d2fb" // BLE: UUID of the Prometeo Service
#define SENSORS_CHARACTERISTIC_UUID "dcaaccb4-c1d1-4bc4-b406-8f6f45df0208" // BLE: UUID for sensors characteristic(all sensors readings in only one characteristic)
#define DATE_TIME_CHARACTERISTIC_UUID "e39c34e9-d574-47fc-a66e-425cec812aab" // BLE: UUID for the Date and Time characteristic, we are going to get this value from the mobile
#define STATUS_CLOUD_CHARACTERISTIC_UUID "125ad2af-97cd-4f7a-b1e2-5109561f740d" // BLE: UUID for the Status Cloud Color characteristic, we are going to get this value from the mobile

// Global Variables definition
// Variables to put the readings from the sensors
uint32_t preHeatSeconds = 30; //30 second preheat advised
uint32_t measureSeconds = 10; //configurable, 10-60 seconds
float tempValue = 0;
float humValue = 0;
float coValue = 0;
float no2Value = 0;
float tempThreshold = 80; //TODO: ask John/team what threshold is
float coThreshold = 2000; //TODO: don't use voltage but actual equation and values
float no2Threshold = 2000; //TODO: don't use voltage but actual equation and values
float tempArray[10];
float coArray[10];
float no2Array[10];
int arrayIndex = 0;
bool afterFirst10Mins = false;
int colourLED = 0; // 1 - green, 2 - yellow, 3 - red 

// sensorValues is the variable where we put readings from all sensors
char sensorValues[40];
String date_time;

//  DHT22 configuration 
#define DHTPIN 27     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   
DHT dht(DHTPIN, DHTTYPE);

//  LED(status) configuration 
#define statusLED_PIN    A0 //(ESP32 could be pin #26) 
#define connectLED_PIN   A1
#define LED_COUNT  1 
#define BRIGHTNESS 50
Adafruit_NeoPixel statusLED(LED_COUNT, statusLED_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel connectLED(LED_COUNT, connectLED_PIN, NEO_GRBW + NEO_KHZ800);
String status_cloud;

//  SD CARD ChipSelect PIN configuration 
#define CS_PIN   33 //4 in some setups, 33 in others

// CJMCU-4541 configuration 
#define PRE_PIN          14 // Replace DigitalPin# by the number where the Pre connection of the CJMCU-4514 is connected
#define VNOX_PIN         36 // Replace A3 by the AnalogPin# you are using
#define VRED_PIN         39 // Replace A4 by the AnalogPin# you are using
//analog read
int co=0;
int no2=0;

//Value voltaje
float vco=0;
float vno2=0;

//Value resistencia
float rco=0;
float rno2=0;

//Value Rs/Ro
float conCO=0;
float conNO2=0;

//ppm Calculation
float ppmCO=0;
float ppmNO2=0;


//  Functions Definition 

// Needed to know if a device is connected or not (deviceConnected flag)
class MyServerCallbacks: public BLEServerCallbacks 
{
      void onConnect(BLEServer* pServer) { deviceConnected = true; };
      void onDisconnect(BLEServer* pServer) { deviceConnected = false; }
};

// With this kind of classes we can identify if the client app is writing a characteristic and we can take actions. 
//In this case we get the Date and Time from the mobile app
class MyCallbacksDateTime: public BLECharacteristicCallbacks 
{
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

// This function is used to get the status color from the mobile
class MyCallbacksStatusCloud: public BLECharacteristicCallbacks 
{
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0) {
      status_cloud = "";
      for (int i = 0; i < value.length(); i++){
        status_cloud = status_cloud + value[i];
      }

      Serial.println("*********");
      Serial.print("Status Color sent by the Prometeo Mobile App: ");
      Serial.println(status_cloud);
      Serial.println("*********");
    }
  }
};


// Function for reading sensor values and taking average over set seconds
void readSensors() 
{
  //Average temp, humidity, and gas values over number of seconds (measureSeconds)
  float sumTemp = 0;
  float sumHum = 0;
  float sumCO = 0;
  float sumNO2 = 0;
  uint32_t count = 0;
  
  while(count < measureSeconds)
  {
     sumTemp = sumTemp + dht.readTemperature(); //temp in Celsius; for temp in Fahrenheit, use dht.readTemperature(true);
     sumHum = sumHum + dht.readHumidity();
     

     //Get Data from sensor CJMCU-4541
     co=analogRead(VRED_PIN ); 
     no2=analogRead(VNOX_PIN); 
      //Convert to voltaje
     vco=(3.3*co)/4096;
     vno2=(3.3*no2)/4096;
      //Convert to resist
     rco=47000*((3.3-vco)/vco);//load resistor in red 1ohm
     rno2=((270*(3.3-vno2))/vno2);//load resistor in ox 270ohm 
     //Convert to indicator concentration
     conCO= 47000/rco;
     conNO2= 270/rno2;
      //PPM Calculation
      ppmCO=(4.4138*pow(conCO,-1.178));
      ppmNO2= ((-0.0003*(conNO2*conNO2))+(0.1626*conNO2)-0.0217);

      sumCO = sumCO + ppmCO;
      sumNO2 = sumNO2 + ppmNO2;
     
     delay(1000);
     count++;
  }
  tempValue = sumTemp / count;
  humValue = sumHum / count;
  coValue = sumCO / count;
  no2Value = sumNO2 / count;
  
  Serial.print("Avg Temperature: ");
  Serial.print(tempValue);
  Serial.print(" # Avg Humidity: ");
  Serial.println(humValue);
  Serial.print("Avg Vco: ");
  Serial.print(coValue); 
  Serial.print(" # Avg Vno2: ");
  Serial.println(no2Value);
  
  // Sprintf with float only works in ESP32, we have to take in account in case we use another processor
  sprintf(sensorValues, "%3.2f %3.2f %3.2f %3.2f", tempValue , humValue, coValue, no2Value);
  
} //end readSensors()

// Procedure to write the data into the SD Card 
// TO-DO: write the values into the SD Card identified by date_time of the first reading. It has to use rotational files 
// We have to add the sent_to_client flag when we write the row in the file to know if we sent or not the data
void writeSDCard(fs::FS &fs, const char * path, const char * message)
{
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
} //end writeSDCard()


void updateStatusLED() 
{
  //Update array values with new data
  tempArray[arrayIndex] = tempValue;
  coArray[arrayIndex] = coValue;
  no2Array[arrayIndex] = no2Value;

  //Get new averages
  float tempSum = 0;
  float coSum = 0;
  float no2Sum = 0;

  if(afterFirst10Mins)
  {
    for(int i=0; i<10; i++)
    {
      tempSum += tempArray[i];
      coSum += coArray[i];
      no2Sum += no2Array[i];
    }

    float tempAvg = tempSum/10;
    float coAvg = coSum/10;
    float no2Avg = no2Sum/10; 

    Serial.print("Temp 10min Avg:");
    Serial.print(tempAvg);
    Serial.print(" CO 10min Avg:");
    Serial.print(coAvg);
    Serial.print(" NO2 10min Avg:");
    Serial.println(no2Avg);

    //Check averages against thresholds
    if((tempAvg > tempThreshold) || (coAvg > coThreshold) || (no2Avg > no2Threshold))
    {
      Serial.println("Value over threshold, display red LED");
      colourLED = 3; // 1 - green, 2 - yellow, 3 - red 
    }
    else colourLED = 1; // green
  }

  if(deviceConnected) // the mobile app has priority
  {
       colourLED = status_cloud.toInt();   // 1 - green, 2 - yellow, 3 - red      
  }

  //Change LED colors
  switch(colourLED) {
      case 3:  // red
          statusLED.setPixelColor(0, statusLED.Color(0, 255, 0)); // Red
          statusLED.show();
          Serial.println("LED colour is red");
          break;  
      case 2: // yellow
          statusLED.setPixelColor(0, statusLED.Color(255, 255, 0)); //Yellow?
          statusLED.show();
          Serial.println("LED colour is yellow");
          break;  
      case 1: // green
          statusLED.setPixelColor(0, statusLED.Color(255, 0, 0)); // Green
          statusLED.show();
          Serial.println("LED colour is green");
          break;  

  }


  //Update array index
  if(arrayIndex < 9)
  {
    arrayIndex++;
  }
  else
  {
    arrayIndex = 0;
    afterFirst10Mins = true;
  }

} //end updateStatusLED()

//initial setup
void setup()
{
  Serial.println("Initial setup - begin");
  Serial.begin(115200); 

  //Define Prometeo ID including MAC Address
  prometeoID = "Prometeo-" + WiFi.macAddress();
  Serial.println("This device is " + prometeoID);
 
  // Create the BLE Device
  BLEDevice::init(prometeoID.c_str());
  pServer = BLEDevice::createServer(); // Create the BLE Server
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService( PROMETEO_SERVICE_UUID ); // Create the BLE Service
  // Create a BLE Characteristic for the sensors readings
  pCharacteristic = pService->createCharacteristic( SENSORS_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE );
  // Create a BLE Characteristic for the date and time, it has a PROPERTY_WRITE because we expect the mobile to write this characteristic
  pDateTime = pService->createCharacteristic( DATE_TIME_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE );
  pDateTime->setCallbacks(new MyCallbacksDateTime()); // Here we indicate the callback function that we'll be called when the Mobile will write the date and time characteristic
    
  // Create a BLE Characteristic for the status color from the cloud, it has a PROPERTY_WRITE because we expect the mobile to write this characteristic
  pStatusCloud = pService->createCharacteristic( STATUS_CLOUD_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE );
  pStatusCloud->setCallbacks(new MyCallbacksStatusCloud()); // Here we indicate the callback function that we'll be called when the Mobile will write the date and time characteristic



  pCharacteristic->addDescriptor(new BLE2902()); // Create BLE Descriptors
  pDateTime->addDescriptor(new BLE2902());
  pStatusCloud->addDescriptor(new BLE2902());
  
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
  
  // TODO: Name file according to timestamp

  uint8_t cardType = SD.cardType();
  delay(5000);

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
      Serial.println("MMC");
  } else if(cardType == CARD_SD){
      Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
      Serial.println("SDHC");
  } else if(cardType == CARD_NONE) {
     Serial.println("No SD card attached");
  } else {
      Serial.println("UNKNOWN");
  }
      
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  delay(5000);
    
  Serial.printf("Total space of SD Card: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space in SD Card: %lluMB\n", SD.usedBytes() / (1024 * 1024));

  // Turn on LED 
  Serial.println("Turning on LEDs");
  connectLED.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  statusLED.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)

  connectLED.setPixelColor(0, connectLED.Color(0, 0, 255)); // Blue
  connectLED.setBrightness(10); // Set BRIGHTNESS low
  connectLED.show();

  statusLED.setPixelColor(0, statusLED.Color(0, 0, 255)); // Blue           
  statusLED.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  statusLED.show();

  // Turn on dht22 temp/hum sensor
  dht.begin();

  // set up MiCS-4514 gas sensor
  pinMode(PRE_PIN, OUTPUT); //set preheat pin
  
  //Preheat gas sensor
  Serial.println("Preheating gas sensor...");
  digitalWrite(PRE_PIN, 1);
  int count = 0;
  while(count < preHeatSeconds)
  {
     delay(1000);
     count++;
  }
  digitalWrite(PRE_PIN, 0);
  Serial.println("Gas sensor preheated.");
 
  Serial.println("Finished initial setup");
  Serial.println("");
  
}// end setup()

void loop()
{
  Serial.println("loop - begin");

  // We read the different sensor values
  readSensors();
 
  // Flag to know if we sent the data to the mobile, so we can store this flag in the SD Card. In case the row in the file in the SD Card has this value false, we can send later
  sent_to_client = false;
    
  if (deviceConnected) 
  {   
    pCharacteristic->setValue(sensorValues);

    // We send a notification to the client that is connected (our Prometeo mobile app)
    pCharacteristic->notify();

    // We put the flag true to know that we sent this data
    sent_to_client = true;
    writeSDCard(SD, "/sensorsReadingData.txt", date_time.c_str()); //Writing date and time in SD card before sensorReading DataLog
    connectLED.show();
  }
  else
  {
    Serial.println("Bluetooth not connected");
    connectLED.setPixelColor(0, connectLED.Color(0, 0, 255)); // Blue
    connectLED.setBrightness(10); // Set BRIGHTNESS low
    connectLED.show();
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

  //update LED color based on thresholds
  updateStatusLED();

  
  Serial.println("Characteristics sent.");

  uint32_t delaySeconds = (60-measureSeconds)*1000;
  delay(delaySeconds);
  Serial.println("loop - end");
  Serial.println(" ");
    
}//end loop
