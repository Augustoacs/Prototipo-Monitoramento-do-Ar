//============================================================================================================================//|
// --- Include Libraries ---
#include <virtuabotixRTC.h>
#include <SPI.h>
#include <SD.h>

//============================================================================================================================//|
// === Define Pins ===

//LED debug pin
int pinLED = 9;

// MH Real Time Clock Module
#define clk 6
#define dat 7
#define rst 8

/* Micro SD card module pins:

 ** MOSI - pin 11
 ** MISO - pin 12
 ** SKC/CLK - pin 13
 ** CS - pin 4 
*/

//============================================================================================================================//|
// === Variables Declaration ===
String newReadingLine = "";
String co2 = "";
String timeStamp="";

bool sdCardWriteStatus = 0;
int lastReadings[3] = {0, 0, 0};

//============================================================================================================================//|
// === Objects Declaration ===

// Creation of the Real Time Clock Object
virtuabotixRTC  myRTC(clk, dat, rst);

// Create myFile object
File myFile;

//============================================================================================================================//|
// === Function Prototype ===
String DS1302();
void storeData(String newLineData);
int readCO2();


//============================================================================================================================//|
// === Initial Setup ===
void setup() {
  Serial.begin(9600);
  pinMode(pinLED, OUTPUT);
  
// === RTC code === //  
  // Set (only first time, then comment it) the current date, and time in the following format:
  // seconds, minutes, hours, day of the week(Sun-Sat:1-7), day of the month, month, year
  // myRTC.setDS1302Time(55, 47, 21, 4, 7, 2, 2024); //no time setup is needed, maintain it commented

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
// === End RTC code === //

// === Micro SD code === //
  Serial.print("Initializing SD card...");

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }//end if
  Serial.println("initialization done.");

  storeData("---");
  
// === End Micro SD code ===//

}//end setup()

//============================================================================================================================//|
// === Initial Loop ===
void loop() {
  co2 = String(readCO2());
  timeStamp = DS1302();
  newReadingLine = timeStamp + ";" + co2;
  storeData(newReadingLine);
  Serial.print(timeStamp);
  Serial.print("\t");
  Serial.print(co2);
  Serial.print("\t");
  Serial.println(newReadingLine);
  delay(2000);
  
}//end loop


//============================================================================================================================//|
// === Functions ===

String DS1302(){
  // This allows for the update of variables for time or accessing the individual elements.                //|
  myRTC.updateTime();
  char timeStamp[20] = {};
  sprintf(timeStamp, "%04d-%02d-%02d %02d:%02d:%02d", myRTC.year, myRTC.month, myRTC.dayofmonth, myRTC.hours, myRTC.minutes, myRTC.seconds); 
  return timeStamp;
}//end DS1302

int readCO2(){
  // Request CO2 concentration data
  byte cmd[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  Serial.write(cmd, sizeof(cmd));

  // Wait for the response (9 bytes)
  delay(100);  // Wait for the sensor to respond
  while (Serial.available() < 9) {}

  // Read and display the CO2 concentration data
  byte response[9];
  Serial.readBytes(response, 9);

  if (response[0] == 0xFF && response[1] == 0x86) {
    unsigned int co2 = (response[2] << 8) | response[3];
    return co2;
  }
}

void storeData(String newLineData){
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open("log.txt", FILE_WRITE);
  // if the file opened okay, write to it:
  if (myFile) {
    //Serial.print("Writing to test.txt...");
    myFile.println(newLineData);
    // close the file:
    myFile.close();
    //Set LED on
    debugLED(pinLED,LOW);
//    Serial.println("done.");
  }//end if
  else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }//end else
  // close the file:
  myFile.close();
}

void debugLED(int pinLED, bool state){
  if (state){
    digitalWrite(pinLED,HIGH);
  } else{
    digitalWrite(pinLED, LOW);
  }
}

