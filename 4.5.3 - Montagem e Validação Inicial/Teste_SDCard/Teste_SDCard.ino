#include <SPI.h>
#include <SD.h>

// Define the CS pin for the SD card module
#define SD_CS 5

String FileName = "/Database.txt"

File file;

void setup() {
  Serial.begin(115200);

  // Initialize SD card
  if (!SD.begin(SD_CS)) {
    Serial.println("Failed to initialize SD card.");
    return;
  }

  Serial.println("SD card initialized.");

  // Open a file for appending
  file = SD.open(FileName, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending.");
    return;
  }

  // Write to the file
  file.println("Hello, SD card!");

  // Close the file
  file.close();

  Serial.println("Data written to file.");

  // Open the file for reading
  file = SD.open(FileName);
  if (!file) {
    Serial.println("Failed to open file for reading.");
    return;
  }

  // Read from the file
  while (file.available()) {
    Serial.write(file.read());
  }

  // Close the file
  file.close();
}

void loop() {
  // Nothing to do here
}
