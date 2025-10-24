
// Replace DigitalPin# by the number where the Pre connection of the CJMCU-4514 is connected
#define PRE_PIN 14

// Replace A3 by the AnalogPin# you are using
#define VNOX_PIN 39
// Replace A4 by the AnalogPin# you are using
#define VRED_PIN 36

#define PRE_HEAT_SECONDS 10

int vnox_value = 0;
int vred_value = 0;

void setup()
{

  // Setup preheater pin
  pinMode(PRE_PIN, OUTPUT);

  // Initialize serial port
  Serial.begin(9600);
  Serial.println("MiCS-4514 Test Read");
  Serial.print("Preheating...");

  // Wait for preheating
  digitalWrite(PRE_PIN, 1);
  delay(PRE_HEAT_SECONDS * 1000);
  digitalWrite(PRE_PIN, 0);
  Serial.println("Done");
}

void loop()
{

  // Read analog values, print them out, and wait
  vnox_value = analogRead(VNOX_PIN);
  vred_value = analogRead(VRED_PIN);
  Serial.print("Vnox: ");
  Serial.print(vnox_value, DEC);
  Serial.print(" Vred: ");
  Serial.println(vred_value, DEC);
  delay(1000);
}
