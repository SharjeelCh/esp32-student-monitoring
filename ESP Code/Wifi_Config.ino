#include "WiFi.h"

// Replace with your Wi-Fi network credentials
const char* ssid = "Enter wifi name";
const char* password = "Enter wifi password";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  // Wait until connected to Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Print the IP address of the ESP32-CAM
  Serial.print("Connected to WiFi. IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Your camera stream code here (usually using ESP32 Camera library)
}
