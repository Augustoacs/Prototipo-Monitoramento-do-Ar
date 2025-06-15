
#define RXD2 16
#define TXD2 17

void setup() {
  
  Serial.begin(115200);
  //Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
}

void loop() {
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
    int co2 = (response[2] << 8) | response[3];
    Serial.print("CO2 Concentration: ");
    Serial.println(co2);
  }

  // Wait before the next reading
  delay(2000);
}
