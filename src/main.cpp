#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_INA219.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>

#define ONE_WIRE_BUS 14
#define TEMPERATURE_PRECISION 9
#define Relay_pompa 26
#define Relay_cooler 27
#define echoPin 23 // pin echo ultrasonic
#define trigPin 19 // pin triger ultrasonic

#define FIREBASE_HOST "https://dbactro-pi-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "juUd2TokbUAWEh2PXAiotj8zePMVkYFUhlWrfsqI"
#define WIFI_SSID "MIUI"
#define WIFI_PASSWORD "marowan1234"

FirebaseData firebaseData;
FirebaseJson Daya1;
FirebaseJson Daya2;
FirebaseJson SuhuPV;
FirebaseJson Vair;
FirebaseJson Kondisi1;
FirebaseJson Kondisi2;

LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_INA219 INA219_1(0x40);
Adafruit_INA219 INA219_2(0x44);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress insideThermometer, outsideThermometer;

float tegangan_1, arus_1, daya_1;
float tegangan_2, arus_2, daya_2;
long duration; // duration of sound wave travel
int distance;  // distance measurement

void setup(void)
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  // Set database read timeout to 1 minute (max 15 minutes)
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  // tiny, small, medium, large and unlimited.
  // Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  Firebase.setwriteSizeLimit(firebaseData, "tiny");

  /*
  This option allows get and delete functions (PUT and DELETE HTTP requests) works for device connected behind the
  Firewall that allows only GET and POST requests.

  Firebase.enableClassicRequest(firebaseData, true);
  */

  // String path = "/data";

  Serial.println("------------------------------------");
  Serial.println("Connected...");

  lcd.init();
  lcd.backlight();

  // start serial port
  Serial.begin(9600);

  // Start up the library
  sensors.begin(); // temperature
  INA219_1.begin();
  INA219_2.begin();

  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // method 1: by index
  if (!sensors.getAddress(insideThermometer, 0))
    Serial.println("Unable to find address for Device 0");
  if (!sensors.getAddress(outsideThermometer, 1))
    Serial.println("Unable to find address for Device 1");

  // set the resolution to 9 bit per device
  sensors.setResolution(insideThermometer, TEMPERATURE_PRECISION);
  sensors.setResolution(outsideThermometer, TEMPERATURE_PRECISION);

  pinMode(Relay_pompa, OUTPUT);
  pinMode(Relay_cooler, OUTPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

// function to print a device's resolution
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print("Resolution: ");
  Serial.print(sensors.getResolution(deviceAddress));
  Serial.println();
}

//   Main function, calls the temperatures in a loop.

void loop(void)
{

  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures();
  Serial.println("DONE");

  /***** SUHU 1 *****/
  float tempC1 = sensors.getTempC(insideThermometer);
  /*
  if(tempC1 == DEVICE_DISCONNECTED_C)
  {
    Serial.println("Error: Could not read temperature data");
    return;
  }
  */

  Serial.print("Temp C_1: ");
  Serial.print(tempC1);
  Serial.print(" C");

  /***** SUHU 2 *****/
  float tempC2 = sensors.getTempC(outsideThermometer);
  /*
  if(tempC2 == DEVICE_DISCONNECTED_C)
  {
    Serial.println("Error: Could not read temperature data");
    return;
  }
  */
  Serial.print("\t\tTemp C_2: ");
  Serial.print(tempC2);
  Serial.print(" C\n\n");

  /***** KENDALI POMPA *****/
  if (tempC1 > 30.00)
  {
    digitalWrite(Relay_pompa, LOW); // Turn ON Relay
    String kondisi_a = "ON";
    Kondisi1.set("/kondisiPendinginan", kondisi_a);
    Firebase.updateNode(firebaseData, "/datasensor", Kondisi1);
  }
  else if (tempC1 <= 29.0)
  {
    digitalWrite(Relay_pompa, HIGH); // Turn OFF Relay
    String kondisi_b = "OFF";
    Kondisi2.set("/kondisiPendinginan", kondisi_b);
    Firebase.updateNode(firebaseData, "/datasensor", Kondisi2);
  }

  /***** KENDALI COOLER *****/
  else if (tempC2 > 30.0)
  {
    digitalWrite(Relay_cooler, LOW); // Turn ON Relay
  }
  else if (tempC2 <= 29.0)
  {
    digitalWrite(Relay_cooler, HIGH); // Turn OFF Relay
  }

  /*****  Sensor INA219  *****/

  tegangan_1 = INA219_1.getBusVoltage_V();
  arus_1 = INA219_1.getCurrent_mA();
  daya_1 = tegangan_1 * (arus_1 / 1000);

  tegangan_2 = INA219_2.getBusVoltage_V();
  arus_2 = INA219_2.getCurrent_mA();
  daya_2 = tegangan_2 * (arus_2 / 1000);

  Serial.print("Tegangan 1 : ");
  Serial.print(tegangan_1);
  Serial.print(" Volt");
  Serial.print("\t\tArus 1 : ");
  Serial.print(arus_1);
  Serial.print(" mAmp");
  Serial.print("\t\tDaya 1 : ");
  Serial.print(daya_1);
  Serial.print(" Watt\n\n");

  Serial.print("Tegangan 2 : ");
  Serial.print(tegangan_2);
  Serial.print(" Volt");
  Serial.print("\t\tArus 2 : ");
  Serial.print(arus_2);
  Serial.print(" mAmp");
  Serial.print("\t\tDaya 2 : ");
  Serial.print(daya_2);
  Serial.print(" Watt\n\n");

  /***** SENSOR ULTRASONIC *****/
  // Clears the trigPin condition
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);

  // Calculating the distance
  distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)
  float kapasitasAir = (975 * (25 - distance)) / 1000;

  // Displays the distance on the Serial Monitor
  Serial.print("Kapasitas Air: ");
  Serial.print(kapasitasAir);
  Serial.println(" L");

  Serial.println("------------------------------------------------------------");

  /***** SEND DATABASE ****/
  Daya1.set("/daya1", daya_1);
  Daya2.set("/daya2", daya_2);
  SuhuPV.set("/suhuPV", tempC1);
  Vair.set("/kapasitasAir", kapasitasAir);
  Firebase.updateNode(firebaseData, "/datasensor", Daya1);
  Firebase.updateNode(firebaseData, "/datasensor", Daya2);
  Firebase.updateNode(firebaseData, "/datasensor", SuhuPV);
  Firebase.updateNode(firebaseData, "/datasensor", Vair);

  /***** TAMPILAN LCD *****/
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Kapasitas Air: ");
  lcd.setCursor(0, 1);
  lcd.print(kapasitasAir);
  lcd.print(" L");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Daya 1: ");
  lcd.setCursor(0, 1);
  lcd.print(daya_1 * 1000);
  lcd.print(" mW");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Daya 2: ");
  lcd.setCursor(0, 1);
  lcd.print(daya_2 * 1000);
  lcd.print(" mW");
  delay(2000);
}