

// WiFi connection
#include <WiFi.h>

// Credentials
#include "secrets.h"

// Network Credentials
char ssid[] = SECRET_SSID;   // your network SSID (name) 
char pass[] = SECRET_PASS;   // your network password

void setup() {
  
    Serial.begin(115200);

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

}

void loop() {
  
}
