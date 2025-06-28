#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <map>
std::map<int, String> previousStatuses;

// Supabase details
const char* ssid = "....";           // Replace with your WiFi SSID
const char* password = "123456789";  // Replace with your WiFi Password
const char* supabaseUrl = "https://jtsxwybsadjlyzjuahwy.supabase.co/rest/v1/student";
const char* supabaseUrl2 = "https://jtsxwybsadjlyzjuahwy.supabase.co/rest/v1/buslocation";

const char* supabaseKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imp0c3h3eWJzYWRqbHl6anVhaHd5Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3MzQxNjcxNjYsImV4cCI6MjA0OTc0MzE2Nn0.W7yXb7uEB7h75RrCp8KP7OQBwkoSjiyXZ8jrIJzTBT4";
bool checkk = true;
// Create an instance of the TinyGPSPlus library
TinyGPSPlus gps;

// Define the serial connection for the GPS module (UART1)
HardwareSerial gpsSerial(1);  // Use UART1 for GPS communication

// Ultrasonic sensor pins
const int trigPin = 32;  // GPIO32
const int echoPin = 33;  // GPIO33

// Buzzer pin
const int buzzerPin = 14;  // GPIO14

// Variable to store the previous status of the student
String previousStatus = "offboard";  // Assuming initially "offboard"
bool isFirstTime = true;             // Flag to check if it's the first boot

void setup() {
  // Initialize Serial Monitor for debugging
  Serial.begin(115200);

  // Initialize GPS Serial communication at 9600 baud rate
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);  // RX: GPIO16, TX: GPIO17
  Serial.println("GPS Module with ESP32 using TinyGPSPlus");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // Initialize ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Initialize buzzer pin
  pinMode(buzzerPin, OUTPUT);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    bool gpsDataAvailable = false;

    // Read GPS data
    while (gpsSerial.available() > 0) {
      char c = gpsSerial.read();
      if (gps.encode(c)) {
        if (gps.location.isUpdated()) {
          double latitude = gps.location.lat();
          double longitude = gps.location.lng();
          Serial.println("GPS Data Available");
          Serial.print("Latitude: ");
          Serial.println(latitude, 6);
          Serial.print("Longitude: ");
          Serial.println(longitude, 6);
          updateGpsForAllOnboardStudents(latitude, longitude);
          gpsDataAvailable = true;  // GPS data is available
          break;                    // Exit after sending data
        }
      }
    }

    // If GPS data is not available, send hardcoded values
    // If GPS data is not available, send hardcoded values
    if (!gpsDataAvailable) {
      Serial.println("GPS Data Not Available");
      double hardcodedLatitude1 = 33.5780558;   // Example latitude for scenario 1
      double hardcodedLongitude1 = 73.0616181;  // Example longitude for scenario 1

      double hardcodedLatitude2 = 33.5790558;   // Example latitude for scenario 2
      double hardcodedLongitude2 = 73.0646181;  // Example longitude for scenario 2
      if (checkk) {
        Serial.println("Using First Set of Hardcoded Coordinates");
        updateGpsForAllOnboardStudents(hardcodedLatitude1, hardcodedLongitude1);
      } else {
        Serial.println("Using Second Set of Hardcoded Coordinates");
        updateGpsForAllOnboardStudents(hardcodedLatitude2, hardcodedLongitude2);
      }

      // Toggle the `checkk` flag
      checkk = !checkk;
    }




    // Check status of student periodically
    checkStatusChange();

    // Ultrasonic sensor and buzzer
    long duration, distance;
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    distance = duration * 0.034 / 2;  // Distance in cm

    // Keep the buzzer on if an object is within 50 cm
    Serial.println(distance);
    if (distance > 50 && distance < 100) {
      Serial.print("Distance: ");
      Serial.print(distance);
      Serial.println(" cm");
      //tone(buzzerPin, 1000); // Keep the buzzer on
    } else {
      //tone(buzzerPin, 0, 500); // Turn off the buzzer
    }
  } else {
    Serial.println("WiFi Disconnected");
  }

  delay(3500);  // Periodically check GPS data and status every 5 seconds
}

void updateGpsForAllOnboardStudents(double latitude, double longitude) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Build the Supabase API URL with a filter for status = 'onboard'
    String url = String(supabaseUrl2) + "?id=eq.1";

    // Set up the request
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", supabaseKey);

    // Create JSON payload
    String payload = "{\"latitude\": " + String(latitude, 6) + ", \"longitude\": " + String(longitude, 6) + "}";

    // Send PATCH request
    int httpResponseCode = http.PATCH(payload);

    // Check the response
    if (httpResponseCode == 204) {
      Serial.println("Update successful, no content returned.");
    } else if (httpResponseCode > 0) {
      Serial.print("HTTP Response Code: ");
      Serial.println(httpResponseCode);
      Serial.println("Response: " + http.getString());
    } else {
      Serial.print("Error in sending PATCH: ");
      Serial.println(http.errorToString(httpResponseCode));

      // Attempt to retry sending the request once
      Serial.println("Retrying PATCH request...");
      delay(1000);  // wait before retrying
      httpResponseCode = http.PATCH(payload);

      // Check again after retry
      if (httpResponseCode > 0) {
        Serial.print("HTTP Response Code after retry: ");
        Serial.println(httpResponseCode);
        Serial.println("Response: " + http.getString());
      } else {
        Serial.print("Failed after retry: ");
        Serial.println(http.errorToString(httpResponseCode));
      }
    }

    http.end();  // End the HTTP session for this request
  } else {
    Serial.println("WiFi Disconnected");
  }
}

void checkStatusChange() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Get the status of the students (IDs 400, 401, 402, 403, 404, and 405)
    String url = String(supabaseUrl) + "?id=in.(400,401,402,403,404,405)";  // Updated to include additional IDs
    http.begin(url);
    http.addHeader("apikey", supabaseKey);

    // Make the GET request
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      String response = http.getString();
      Serial.print("Supabase Response: ");
      Serial.println(response);

      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, response);

      if (error) {
        Serial.print("JSON Parsing Error: ");
        Serial.println(error.c_str());
        return;
      }

      // Iterate through the student data
      JsonArray students = doc.as<JsonArray>();
      for (JsonObject student : students) {
        int studentId = student["id"];
        String currentStatus = student["status"].as<String>();

        // First-time initialization
        if (previousStatuses.find(studentId) == previousStatuses.end()) {
          previousStatuses[studentId] = currentStatus;
          Serial.print("Initialized status for Student ID ");
          Serial.print(studentId);
          Serial.print(": ");
          Serial.println(currentStatus);
          continue;
        }

        // Check for status change
        if (currentStatus != previousStatuses[studentId]) {
          Serial.print("Status changed for Student ID ");
          Serial.print(studentId);
          Serial.print(" to: ");
          Serial.println(currentStatus);

          // Trigger the buzzer
          tone(buzzerPin, 1000, 1500);  // Buzzer on for 1.5 seconds

          // Update the previous status
          previousStatuses[studentId] = currentStatus;
        }
      }
    } else {
      Serial.print("HTTP GET Request Failed: ");
      Serial.println(httpResponseCode);
    }

    http.end();  // End the HTTP session
  } else {
    Serial.println("WiFi Disconnected");
  }
}

