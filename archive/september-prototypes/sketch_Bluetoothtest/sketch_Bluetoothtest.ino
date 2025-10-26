#include "BluetoothSerial.h"
BluetoothSerial SerialBT;
void setup()
{
  SerialBT.begin("ESP32");
}
void loop()
{
  SerialBT.println("hello world");
  delay(1000);
}
