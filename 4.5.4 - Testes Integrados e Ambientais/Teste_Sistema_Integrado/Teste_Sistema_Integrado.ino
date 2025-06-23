



//========================================================================||
// === INCLUDING LIBRARIES ===
#include <virtuabotixRTC.h> // RTC library
#include <SPI.h>            // SPI protocol
#include <SD.h>             // SD card library
#include <Adafruit_MPL3115A2.h> // MPL3115A2 Temp and Baro sensor library

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


// === VARIABLES DECLARATION ===
const int loopInterval = 2000;
long unsigned int loopLastMillis = 0;
long unsigned int loopCurrentMillis = 0;
bool error = false;
bool errorCO2 = false;
bool errorSDCard = false;
bool errorInitialize = false;
bool errorInitializeSDCard = false;
bool errorInitializeMPL3115A2 = false;
String newReadingLine = "";
String co2 = "";
String temperature = "";
String atmPressure = "";
String timeStamp = "";
String myDocumentFile = "/Database.txt";


// === FUNCTIONS PROTOTYPES  ===
// Error indicator
void blinkErrorLED();

// DS1302 RTC module
String DS1302();

// MZH19B CO2 sensor
int readCO2();

// MPL3115A2 Temperature and Pressure sensor
float getPressure();
float getTemperature();

// SD Card Module
void storeData();


// === OBJECTS DECLARATION ===
virtuabotixRTC  myRTC(clk, dat, rst); // Real Time Clock Object
File myFile; // Create myFile object
Adafruit_MPL3115A2 baro;

//========================================================================||
// === INITIAL SETUP ===
void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); // Serial communication for MHZ19b
  
  pinMode(builtin_LED,OUTPUT);

  // DS1302
  // Set the current date, and time in the following format:
  // seconds, minutes, hours, day of the week(Sun-Sat:1-7), day of the month, month, year
  // myRTC.setDS1302Time(0, 47, 21, 4, 7, 2, 2024); //no time setup is needed, maintain it commented
  
//  while (!Serial && !Serial2) {
//    ; // wait for serial port to connect. Needed for native USB port only
//  }

  // MPL3115A2
  // Initializing and testing Barometer and Temperature sensor
  if (!baro.begin()) {
    Serial.println("Could not find sensor. Check wiring.");
    errorInitializeMPL3115A2 = true;
  }else{
    errorInitializeMPL3115A2 = false;
  }// end else

  // SD Card
  // Initializing and testing SD Card
  if (!SD.begin(CS_pin)) {
    Serial.println("initialization failed!");
    errorInitializeSDCard = true;
  }else{
    Serial.println("SD card initialized.");
    errorInitializeSDCard = false;
  }// end else
  storeData("---",myDocumentFile); // 

  
}// end setup

//========================================================================||
// === INFINITE LOOP ===
void loop() {
  blinkErrorLED(builtin_LED);
  loopCurrentMillis = millis();

  if(loopCurrentMillis - loopLastMillis >= loopInterval && loopLastMillis < loopCurrentMillis){
    co2 = String(readCO2());
    temperature = String(getTemperature());
    atmPressure = String(getPressure());
    timeStamp = DS1302();
    newReadingLine = timeStamp + ";" + co2 + ";" + temperature + ";" + atmPressure;
    storeData(newReadingLine, myDocumentFile);
    
    if(co2.toInt() > 5000 || co2.toInt() < 0){  
      errorCO2 = true;
    } else {
      errorCO2 = false;
    }// end else
    
//    Serial.print(timeStamp);
//    Serial.print("\t");
//    Serial.print(co2);
//    Serial.print("\t");
//    Serial.print(temperature);
//    Serial.print("\t");
//    Serial.println(atmPressure);
    Serial.println(newReadingLine);

    loopLastMillis = millis();
  } else if(loopLastMillis > loopCurrentMillis){
    loopLastMillis = millis();
  } else {
    ;
  }// end else
  
}// end loop

//========================================================================||
// === FUNCTIONS DECLARATION ===

void blinkErrorLED(int pinLED){
  static unsigned long errorPreviousMillis = 0; // Variable to store last blink
  const unsigned int interval = 500; // Interval between blink
  static bool state = LOW; // State of the LED
  unsigned long errorCurrentMillis = millis();
  error = errorCO2 || errorSDCard || errorInitialize || errorInitializeSDCard || errorInitializeMPL3115A2;
  

  if(error == false){
    state = LOW;
    digitalWrite(pinLED, state);
  }else if(errorCurrentMillis - errorPreviousMillis >= interval && errorCurrentMillis > errorPreviousMillis) {
    state = !state; // Toggle the LED state
    digitalWrite(pinLED, state); // Update the LED
    errorPreviousMillis = millis(); // Save the last time the LED was toggled
  }else if(errorCurrentMillis < errorPreviousMillis){
    errorPreviousMillis = millis(); // Save the last time the LED was toggled
  }else{
    ;
  }
}// end blinkErrorLED


String DS1302(){
  // This allows for the update of variables for time or accessing the individual elements.                //|
  myRTC.updateTime();
  char timeStamp[20] = {};
  sprintf(timeStamp, "%04d-%02d-%02d %02d:%02d:%02d", myRTC.year, myRTC.month, myRTC.dayofmonth, myRTC.hours, myRTC.minutes, myRTC.seconds); 
  return timeStamp;
}// end DS1302


int readCO2(){
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

void storeData(String newLineData, String documentFile){
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
