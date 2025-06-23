//========================================================================||

//   === 1. Libraries, GPIOs, Variables, Prototypes,Objects ===

//========================================================================||
// === INCLUDING LIBRARIES ===

// WiFi connection
#include <WiFi.h>

// RTC library
#include <virtuabotixRTC.h>

// Credentials
#include "secrets.h"

// SPI protocol
#include <SPI.h>

// SD card library
#include <SD.h>
// MPL3115A2 Temp and Baro sensor library
#include <Adafruit_MPL3115A2.h>

// Firebase communication
#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>

//========================================================================||
// === VARIABLES DECLARATION ===

// UTC time location (in hours)
const int UTC = -3;

// Millis control variables
const int loopInterval = 5000;
long unsigned int loopLastMillis = 0;
long unsigned int loopCurrentMillis = 0;

// Erros indicators variables
bool error = false;
bool errorCO2 = false;
bool errorSDCard = false;
bool errorFirebase = false;
bool errorInitialize = false;
bool errorInitializeSDCard = false;
bool errorInitializeMPL3115A2 = false;

// Readings storage variables
String newReadingLine = "";
String co2 = "";
String temperature = "";
String atmPressure = "";
String timestampSample = "";
String myDocumentFile = "/Database.txt";

//========================================================================||
// === DEFINE GPIO ===

// Built in LED
#define builtin_LED 2

// DS1302 Real Time Clock module (3.3V)
#define clk 26
#define dat 27
#define rst 25

// MHZ19b CO2 sensor (5V)
#define RXD2 16
#define TXD2 17

// MPL3115A2 Temperature and Pressure sensor (3.3V)
//#define SCL 22
//#define SCA 21

// Micro SD card module pins (5V)
//#define MOSI 23
//#define MISO 19
//#define SKC/CLK 18
#define CS_pin 5


//========================================================================||
// === FUNCTIONS PROTOTYPES  ===
// Error indicator
void blinkErrorLED();

// DS1302 RTC module
String DS1302_GetDateAndTime();

// MZH19B CO2 sensor
int readCO2();

// MPL3115A2 Temperature and Pressure sensor
float getPressure();
float getTemperature();

// SD Card Module
void storeData();

//========================================================================||
// === OBJECT DECLARATION ===
// Define Firebase Data object
FirebaseData fbdo;

// Define RTC object
virtuabotixRTC myRTC(clk, dat, rst);

// Create myFile object
File myFile;

// Barometer and Temperature sensor object
Adafruit_MPL3115A2 baro;

// Firebase authentication and configuration objects
FirebaseAuth auth;
FirebaseConfig config;


//========================================================================||

//   === 2. SETUP ===

//========================================================================||
// === INITIAL SETUP ===
void setup()
{

  Serial.begin(115200);

  // Serial communication for MHZ19b
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  pinMode(builtin_LED, OUTPUT);

  // WiFi connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  unsigned long ms = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

 // Inform Firebase client library version
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;


  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback;

  // Reconnect WiFi if connection was lost
  Firebase.reconnectNetwork(true);

  // Initialize Firebase with the configuration and authentication information
  Firebase.begin(&config, &auth);

  // MPL3115A2
  // Initializing and testing Barometer and Temperature sensor
  if (!baro.begin()) {
    Serial.println("Could not find sensor. Check wiring.");
    errorInitializeMPL3115A2 = true;
  } else {
    errorInitializeMPL3115A2 = false;
  }// end else

  // SD Card
  // Initializing and testing SD Card
  if (!SD.begin(CS_pin)) {
    Serial.println("initialization failed!");
    errorInitializeSDCard = true;
  } else {
    Serial.println("SD card initialized.");
    errorInitializeSDCard = false;
  }// end else
  storeData("---", myDocumentFile); //

}

//========================================================================||

//   === 3. LOOP ===

//========================================================================||
//   === INFINITE LOOP ===
void loop()
{
  blinkErrorLED(builtin_LED);
  loopCurrentMillis = millis();

  timestampSample = DS1302_GetDateAndTime();

  // Firebase.ready() should be called repeatedly to handle authentication tasks.

  if (Firebase.ready() && loopCurrentMillis - loopLastMillis >= loopInterval && loopLastMillis < loopCurrentMillis)
  {
    loopLastMillis = millis();

    co2 = String(readCO2());
    temperature = String(getTemperature());
    atmPressure = String(getPressure());
    timestampSample = DS1302_GetDateAndTime();

    newReadingLine = timestampSample + ";" + co2 + ";" + temperature + ";" + atmPressure;

    storeData(newReadingLine, myDocumentFile);

    if (co2.toInt() > 5000 || co2.toInt() < 0) {
      errorCO2 = true;
    } else {
      errorCO2 = false;
    }// end else

    // Firebase data
    FirebaseJson content;

    String documentPath = "collection2/" + timestampSample;

    // integer
    content.set("fields/CO2_Concentration/integerValue", co2);
    // integer
    content.set("fields/Temperature/doubleValue", temperature);
    // integer
    content.set("fields/AtmPressure/doubleValue", atmPressure);
    // timestampSample
    content.set("fields/timestampSample/timestampValue", timestampSample + "Z"/*"2014-09-21T15:01:23Z"*/);

    Serial.print("Create a document... ");

    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())) {
      //            Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
      Serial.println(fbdo.payload().c_str());
      errorFirebase = false;
    } else {
      Serial.println(fbdo.errorReason());
      errorFirebase = true;
    }// end else

  } else if (loopLastMillis >= loopCurrentMillis) {
    loopLastMillis = millis();
  } else {
    ;
  }// end else
}



//========================================================================||

//   === 4. FUNCTIONS ===

//========================================================================||
// === FUNCTIONS DECLARATION ===

void blinkErrorLED(int pinLED) {
  static unsigned long errorPreviousMillis = 0; // Variable to store last blink
  const unsigned int interval = 500; // Interval between blink
  static bool state = LOW; // State of the LED
  unsigned long errorCurrentMillis = millis();
  error = errorCO2 || errorSDCard || errorInitialize || errorInitializeSDCard || errorInitializeMPL3115A2 || errorFirebase;

  if (error == false) {
    state = LOW;
    digitalWrite(pinLED, state);
    
//    Serial.println("++++");
//    Serial.println("error: " + (String)error);
//    Serial.println("errorCO2: " + (String)errorCO2);
//    Serial.println("errorSDCard: " + (String)errorSDCard);
//    Serial.println("errorInitialize: " + (String)errorInitialize);
//    Serial.println("errorInitializeSDCard: " + (String)errorInitializeSDCard);
//    Serial.println("errorInitializeMPL3115A2: " + (String)errorInitializeMPL3115A2);
//    Serial.println("errorFirebase: " + (String)errorFirebase);
//    Serial.println("state: " + (String) state);
//    Serial.println("errorCurrentMillis: " + (String) errorCurrentMillis);
//    Serial.println("errorPreviousMillis: " + (String) errorPreviousMillis);

  } else if (errorCurrentMillis - errorPreviousMillis >= interval && errorCurrentMillis > errorPreviousMillis) {
    state = !state; // Toggle the LED state
    digitalWrite(pinLED, state); // Update the LED
    errorPreviousMillis = millis(); // Save the last time the LED was toggled

//    Serial.println("----");
//    Serial.println("error: " + (String)error);
//    Serial.println("errorCO2: " + (String)errorCO2);
//    Serial.println("errorSDCard: " + (String)errorSDCard);
//    Serial.println("errorInitialize: " + (String)errorInitialize);
//    Serial.println("errorInitializeSDCard: " + (String)errorInitializeSDCard);
//    Serial.println("errorInitializeMPL3115A2: " + (String)errorInitializeMPL3115A2);
//    Serial.println("errorFirebase: " + (String)errorFirebase);
//    Serial.println("state: " + (String) state);
//    Serial.println("errorCurrentMillis: " + (String) errorCurrentMillis);
//    Serial.println("errorPreviousMillis: " + (String) errorPreviousMillis);
    
  } else if (errorCurrentMillis < errorPreviousMillis) {
    errorPreviousMillis = millis(); // Save the last time the LED was toggled
  } else {
    ;
  }
}// end blinkErrorLED


String DS1302_GetDateAndTime() {
  myRTC.updateTime();
  char timestampSample[20] = {};
  sprintf(timestampSample, "%04d-%02d-%02dT%02d:%02d:%02d", myRTC.year, myRTC.month, myRTC.dayofmonth, myRTC.hours - UTC, myRTC.minutes, myRTC.seconds);
  return timestampSample;
}


int readCO2() {
  // Request CO2 concentration data
  byte cmd[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  Serial2.write(cmd, sizeof(cmd));

  // Wait for the response (9 bytes)
  delay(100);  // Wait for the sensor to respond
  while (Serial2.available() < 9) {}

  // Read and display the CO2 concentration data
  byte response[9];
  Serial2.readBytes(response, 9);

  if (response[0] == 0xFF && response[1] == 0x86) {
    unsigned int co2 = (response[2] << 8) | response[3];
    return co2;
  }
}

float getPressure() {
  return baro.getPressure();
}

float getTemperature() {
  return baro.getTemperature();
}

void storeData(String newLineData, String documentFile) {
  // open the file.
  myFile = SD.open(documentFile, FILE_APPEND);

  // if the file opened okay, write to it:
  if (myFile) {
    errorSDCard = false;
    //Serial.print("Writing to document...");
    myFile.println(newLineData);
    // close the file:
    myFile.close();
  }//end if
  else {
    // if the file didn't open, print an error:
    Serial.println("error opening document");
    errorSDCard = true;
  }//end else

  // close the file:
  myFile.close();
}// end storeData
