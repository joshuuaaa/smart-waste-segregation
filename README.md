# ♻️ Smart Waste Segregation System

An IoT-based intelligent waste segregation system using ESP32-CAM, deep learning, and a smart servo control mechanism. Designed to classify and sort waste into plastic, metal, paper, or others — promoting sustainable and automated waste management.

---

## 🚀 Project Overview

This project aims to build a smart dustbin that:
- Detects when waste is dropped
- Captures an image using ESP32-CAM
- Sends the image to a local Flask server
- Classifies the waste using a CNN model
- Controls a 360° servo motor to segregate the waste accordingly
- Monitors bin fill level using an ultrasonic sensor

---

## 🧩 Components Used

| Hardware               | Description                            |
|------------------------|----------------------------------------|
| ESP32-CAM              | Captures waste images                  |
| ESP32 (separate unit)  | Controls servo & sensors               |
| Ultrasonic Sensor      | Detects waste presence & fill level    |
| Servo Motor (360°)     | Rotates bin flap for sorting           |
| Laptop (Server)        | Hosts Flask API + ML model             |

| Software               | Description                            |
|------------------------|----------------------------------------|
| Flask                  | Python web server                      |
| TensorFlow / Keras     | CNN model for waste classification     |
| Arduino IDE            | For programming ESP32/ESP32-CAM        |
| Python                 | Backend and inference logic            |

---

## 🛠 Project Structure

