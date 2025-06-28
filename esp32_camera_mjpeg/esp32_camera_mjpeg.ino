#include "OV2640.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>

// Select camera model
#define CAMERA_MODEL_AI_THINKER

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define SSID1 "Enter wifi name here"
#define PWD1 "Enter wifi password here"

#define LED_PIN 4 // GPIO pin for onboard LED (adjust if needed)

OV2640 cam;
WebServer server(80);

const char HEADER[] = "HTTP/1.1 200 OK\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "Content-Type: multipart/x-mixed-replace; boundary=123456789000000000000987654321\r\n";
const char BOUNDARY[] = "\r\n--123456789000000000000987654321\r\n";
const char CTNTTYPE[] = "Content-Type: image/jpeg\r\nContent-Length: ";
const int hdrLen = strlen(HEADER);
const int bdrLen = strlen(BOUNDARY);
const int cntLen = strlen(CTNTTYPE);

void handle_jpg_stream()
{
  char buf[32];
  int s;

  WiFiClient client = server.client();

  client.write(HEADER, hdrLen);
  client.write(BOUNDARY, bdrLen);

  while (true)
  {
    if (!client.connected()) break;
    cam.run();
    s = cam.getSize();
    client.write(CTNTTYPE, cntLen);
    sprintf(buf, "%d\r\n\r\n", s);
    client.write(buf, strlen(buf));
    client.write((char *)cam.getfb(), s);
    client.write(BOUNDARY, bdrLen);
  }
}

void handleNotFound()
{
  server.send(404, "text/plain", "404: Not Found");
}

void handleToggleLED()
{
  static bool ledState = false;
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  server.send(200, "text/plain", ledState ? "LED ON" : "LED OFF");
}

void setup()
{
  Serial.begin(115200);

  // LED setup
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Camera configuration
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Improved video quality
  config.frame_size = FRAMESIZE_VGA; // Lower resolution
  // config.frame_size = FRAMESIZE_SVGA; // Higher resolution
  config.jpeg_quality = 12;         // Lower number = better quality
  config.fb_count = 2;

  cam.init(config);

  // Flip camera feed
  cam.setVerticalFlip(true);  // Flip vertically
  // cam.setHorizontalFlip(true); // Flip horizontally

  // Wi-Fi setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID1, PWD1);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("Stream Link: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/mjpeg/1");

  // Server routes
  server.on("/mjpeg/1", HTTP_GET, handle_jpg_stream);
  server.on("/toggleLED", HTTP_GET, handleToggleLED);
  server.onNotFound(handleNotFound);

  server.begin();
}

void loop()
{
  server.handleClient();
}
