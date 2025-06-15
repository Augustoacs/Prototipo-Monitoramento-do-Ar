//========================================================================||

//   === 1. Libraries, GPIOs, Variables, Prototypes,Objects ===

//========================================================================||
// === INCLUDING LIBRARIES ===

// WiFi connection
#include <WiFi.h>

// Credentials
#include "secrets.h"

// ThingSpeak
#include "ThingSpeak.h"

// RTC library
#include <virtuabotixRTC.h>

// SPI protocol
#include <SPI.h>

// SD card library
#include <SD.h>

// MPL3115A2 Temp and Baro sensor library
#include <Adafruit_MPL3115A2.h>

// HTTP Connection
#include <HTTPClient.h>


//========================================================================||
// === VARIABLES DECLARATION ===

// UTC time location (in hours)
const int UTC = -3;

// Millis control variables
const int loopInterval = 300000;
long unsigned int loopLastMillis = 0;
long unsigned int loopCurrentMillis = 0;

/*
// Erros indicators variables
bool error = false;
bool errorCO2 = false;
bool errorSDCard = false;
bool errorInitialize = false;
bool errorInitializeSDCard = false;
bool errorInitializeMPL3115A2 = false;
*/

// Readings storage variables
String newReadingLine = "";
String co2 = "";
String temperature = "";
String atmPressure = "";
String timestampSample = "";
String myDocumentFile = "/Database.txt";


// Network Credentials
char ssid[] = SECRET_SSID;   // your network SSID (name) 
char pass[] = SECRET_PASS;   // your network password


// ThingSpeak Credentials
unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

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

// MPL3115A2 Temperature and Pressure sensor (3.3V) - Set by default
//#define SCL 22
//#define SCA 21

// Micro SD card module pins (5V) - Set by default
//#define MOSI 23
//#define MISO 19
//#define SKC/CLK 18
#define CS_pin 5

//========================================================================||
// === FUNCTIONS PROTOTYPES  ===
// Error indicator

// Telegram Bot
void sendTelegramMessage();

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

// Define RTC object
virtuabotixRTC myRTC(clk, dat, rst);

// Create myFile object
File myFile;

// Barometer and Temperature sensor object
Adafruit_MPL3115A2 baro;

//Wifi Client object
WiFiClient  client;


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
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  WiFi.mode(WIFI_STA); 

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
  sendTelegramMessage("Wifi Connected.");

  // Initialize ThingSpeak
  ThingSpeak.begin(client); 

  // MPL3115A2
  // Initializing and testing Barometer and Temperature sensor
  if (!baro.begin()) {
    Serial.println("Could not find sensor. Check wiring.");
    sendTelegramMessage("Error initializing MPL3115A2.");
  } 


  // SD Card
  // Initializing and testing SD Card
  if (!SD.begin(CS_pin)) {
    Serial.println("initialization failed!");
    sendTelegramMessage("Error initializing SD card.");
  } 
  storeData("---", myDocumentFile); //
}


//========================================================================||

//   === 3. LOOP ===

//========================================================================||
//   === INFINITE LOOP ===
void loop()
{
  loopCurrentMillis = millis();

  // Get TimeStamp
  timestampSample = DS1302_GetDateAndTime();
  if (loopCurrentMillis - loopLastMillis >= loopInterval && loopLastMillis < loopCurrentMillis)
  {
    loopLastMillis = millis();

    co2 = String(readCO2());
    temperature = String(getTemperature());
    atmPressure = String(getPressure());
    timestampSample = DS1302_GetDateAndTime();

    newReadingLine = timestampSample + ";" + co2 + ";" + temperature + ";" + atmPressure;

    storeData(newReadingLine, myDocumentFile);

    if (co2.toInt() > 5000 || co2.toInt() < 0) {
      sendTelegramMessage("CO2 Sensor Error! Value: " + co2);
    }
    else{
      // set the fields with the values
      ThingSpeak.setField(1, co2.toFloat());
      ThingSpeak.setField(2, temperature.toFloat());
      ThingSpeak.setField(3, atmPressure.toFloat());
    
      // write to the ThingSpeak channel
      int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
      if(x == 200){
        Serial.println("Channel update successful.");
        sendTelegramMessage("Data sent to ThingSpeak.");
      }
      else{
        Serial.println("Problem updating channel. HTTP error code " + String(x));
      }
    }
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
  else{
    return -1;
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
    //Serial.print("Writing to document...");
    myFile.println(newLineData);
    // close the file:
    myFile.close();
  }//end if
  else {
    // if the file didn't open, print an error:
    Serial.println("error opening document");
    sendTelegramMessage("SD Card - error opening document.");
  }//end else

  // close the file:
  myFile.close();
}// end storeData


void sendTelegramMessage(String message) {
  HTTPClient http;

  String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatID + "&text=" + message;

  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    Serial.println("Message sent successfully");
  } else {
    Serial.println("Error sending message");
  }
  http.end();
}
