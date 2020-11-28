/***************************************************************************
Source: https://myscope.net/auswertung-der-airpi-gas-sensoren/
 ***************************************************************************/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "DHT.h"
#include "SD.h"
#include "SPI.h"
#include "Adafruit_NeoPixel.h"

#define PRE_PIN          14 // Replace DigitalPin# by the number where the Pre connection of the CJMCU-4514 is connected
#define VNOX_PIN         36 //  A4(36). Replace VNOX_PIN by the AnalogPin# you are using.
#define VRED_PIN         39 // A3(39). Replace RED_PIN by the AnalogPin# you are using. 

//analog read
int co=0;
int no2=0;

//Value voltaje
float vCO=0;
float vNO2=0;

//Value resistencia RO
float r0CO=41300; ///assumed value, pending to be calibrated
float r0NO2=1900; //assumed value, pending to be calibrated


//Value resistencia RS
float rsCO=0;
float rsNO2=0;

//Ratio Rs/Ro
float ratioCO=0;
float ratioNO2=0;

//cálculo de ppm 
double ppmCO=0;
double ppmNO2=0;

void setup() {
  //resolution analog reads
  analogReadResolution(12);
  Serial.begin(115200);
  //pre heating => PIN PRE_PIN(PRE_PIN is PIN 14)
  while(!Serial)
  pinMode(PRE_PIN,OUTPUT);
  Serial.println("pre heating");
  digitalWrite(PRE_PIN, HIGH);
  //delay (30000);
  Serial.println("pre heating done");
  digitalWrite(PRE_PIN, LOW);
}

void loop() {
  
  //Get Sensor Data
  co=analogRead(VRED_PIN ); 
  no2=analogRead(VNOX_PIN); 
  
  //Convert to voltaje
  vCO=(3.3*co)/4096;  // ESP32 Feather trabaja a 3.3volts y 12bits de resolución, de ahí el número 4096
  vNO2=(3.3*no2)/4096;
  Serial.println("Medición de NO2 ");
  Serial.print("Voltage leído en PIN NO2 ");
  Serial.println(vNO2);
  
  //Convert to resist RS
  rsCO=47000*((3.3-vCO)/vCO);//load resistor 47kohm
  rsNO2=22000*(3.3-vNO2)/vNO2;//load resistor 22kohm
  Serial.print("RS leida en NO2 ");
  Serial.println(rsNO2);
  
 //Ratio Rs/RO (Convert to indicator concentration)
  ratioCO= rsCO/r0CO;
  ratioNO2= rsNO2/r0NO2;
  Serial.print("Ratio de NO2 RS/R0 ");
  Serial.println(ratioNO2);
  
  //PPM Calculation
  ppmCO=pow(10, -1.1859 * log10(ratioCO) + 0.6201); //aca agregué LN10 sólo en CO
  ppmNO2 = pow(10, 0.9682 * (log10(ratioNO2)) - 0.8108); 

  Serial.println("Medición de CO: ");
  Serial.print("Voltage leído en PIN CO ");
  Serial.println(vCO);
  Serial.print("RS leida en CO ");
  Serial.println(rsCO);
  Serial.print("Ratio de CO RS/R0 ");
  Serial.println(ratioCO);
  Serial.print("* PPM CO");
  Serial.print(" ");
  Serial.print(ppmCO);
  Serial.print(" PPM NO2 ");
  Serial.println(ppmNO2);
  Serial.println("=======================================");
  
 
  delay(3000);
}
