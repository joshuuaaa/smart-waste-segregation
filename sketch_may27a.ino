#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include "esp_camera.h"
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <WebServer.h>

// ESP32-CAM Pin Definitions
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

// Ultrasonic sensor pins
#define TRIG_PIN 12
#define ECHO_PIN 13

// Servo motor pins
#define SORTING_SERVO_PIN 14
#define DOOR_SERVO_PIN    15

Servo sortingServo;
Servo doorServo;

// WiFi credentials
const char* ssid = "webs";
const char* password = "12345679";

// Flask server URL
String serverUrl = "ipaddress";

// Web server
WebServer server(80);

// Continuous rotation servo constants
#define SERVO_STOP 1500
#define SERVO_CW   1300
#define SERVO_CCW  1700
#define DEGREES_PER_SECOND 96

// Door timing
#define DOOR_OPEN_TIME 6000
#define DOOR_CLOSE_TIME 3000

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  int retries = 30;
  while (WiFi.status() != WL_CONNECTED && retries-- > 0) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi Connection Failed!");
  }
}

void rotateDegrees(int degrees) {
  int rotationTime = abs(degrees) * (1000 / DEGREES_PER_SECOND);
  if (degrees > 0) {
    sortingServo.writeMicroseconds(SERVO_CW); // CW
  } else {
    sortingServo.writeMicroseconds(SERVO_CCW); // CCW
  }
  delay(rotationTime);
  sortingServo.writeMicroseconds(SERVO_STOP);
}

void moveServo(String wasteType) {
  int targetAngle = 0;
  if (wasteType == "Plastic") targetAngle = 0;
  else if (wasteType == "Metal") targetAngle = 90;
  else if (wasteType == "Paper") targetAngle = 180;
  else targetAngle = 270;

  Serial.print("Moving sorting servo to: ");
  Serial.println(targetAngle);

  rotateDegrees(targetAngle);
  doorServo.write(0); // Open drop door
  delay(DOOR_OPEN_TIME);
  doorServo.write(180); // Close door
  delay(DOOR_CLOSE_TIME);
  rotateDegrees(-targetAngle); // Reset to 0°
  Serial.println("Sorting complete. Servo reset to 0°.");
}

void handleServoRequest() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"Bad Request\"}");
    return;
  }
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }
  String wasteType = doc["waste_type"].as<String>();
  Serial.println("Received request to move servo for: " + wasteType);
  moveServo(wasteType);
  server.send(200, "application/json", "{\"status\":\"Servo moved\"}");
}

float getDistance() {
  long sum = 0;
  int validReadings = 0;
  for (int i = 0; i < 5; i++) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    if (duration > 0) {
      sum += duration;
      validReadings++;
    }
  }
  if (validReadings == 0) return -1;
  float distance = (sum / validReadings) * 0.0343 / 2;
  return (distance > 0 && distance < 400) ? distance : -1;
}

String sendImageToServer() {
  WiFiClient client;
  HTTPClient http;
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Failed to capture image");
    return "Error";
  }

  if (!http.begin(client, serverUrl)) {
    Serial.println("Failed to connect to server");
    esp_camera_fb_return(fb);
    return "Error";
  }

  http.addHeader("Content-Type", "image/jpeg");
  int httpResponseCode = http.POST(fb->buf, fb->len);
  String response = http.getString();
  esp_camera_fb_return(fb);
  http.end();

  if (httpResponseCode <= 0) {
    Serial.println("HTTP request failed: " + String(httpResponseCode));
    return "Error";
  }

  return response;
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  sortingServo.attach(SORTING_SERVO_PIN);
  doorServo.attach(DOOR_SERVO_PIN);
  sortingServo.writeMicroseconds(SERVO_STOP);
  doorServo.write(180); // Closed position
  delay(1000);

  connectWiFi();

  server.on("/servo", HTTP_POST, handleServoRequest);
  server.begin();
  Serial.println("ESP32 Web Server started!");

  // Camera Config
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera initialization failed! Restarting...");
    delay(5000);
    ESP.restart();
  } else {
    Serial.println("Camera Ready!");
  }
}

void loop() {
  server.handleClient();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected! Reconnecting...");
    connectWiFi();
    delay(5000);
    return;
  }

  float distance = getDistance();
  if (distance > 0 && distance < 10) {
    Serial.println("Object detected! Capturing image...");
    String response = sendImageToServer();
    if (response == "Error") {
      Serial.println("Failed to send image or receive response.");
      return;
    }

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
      Serial.println("Failed to parse server response: " + String(error.c_str()));
      return;
    }

    String wasteType = doc["class"].as<String>();
    if (wasteType.isEmpty()) {
      Serial.println("Invalid waste type received from server.");
      return;
    }

    Serial.println("Waste classified as: " + wasteType);
    delay(2000);
    moveServo(wasteType);
    delay(5000);
  }

  delay(200);
}
