from flask import Flask, request, jsonify
import os
import requests  # For sending servo control commands to ESP32
import tensorflow as tf
import numpy as np
from PIL import Image
import time  # For delay to avoid race conditions

app = Flask(__name__)

# Folder to store uploaded images
UPLOAD_FOLDER = "Z:\\project\\upload"
os.makedirs(UPLOAD_FOLDER, exist_ok=True)  # Ensure the folder exists

# Load the trained deep learning model
MODEL_PATH = "C:\\Users\\joshu\\Downloads\\waste_classifier.h5"  # Adjust the path
model = tf.keras.models.load_model(MODEL_PATH)

# Define class labels
CLASS_LABELS = ["Metal", "Paper", "Plastic", "Other"]

# ESP32 IP Address (Change this to match your ESP32's actual IP)
ESP32_IP = "http://192.168.209.98"  # Adjust as needed

def preprocess_image(image_path):
    """Loads and preprocesses the image for model inference."""
    try:
        img = Image.open(image_path).convert("RGB")
        img = img.resize((224, 224))  # Resize to match model input shape
        img = np.array(img) / 255.0  # Normalize pixel values
        img = np.expand_dims(img, axis=0)  # Add batch dimension
        return img
    except Exception as e:
        print(f"Error processing image: {e}")
        return None

def control_servo(class_label):
    """Send a request to ESP32 to control the servo motor based on classification."""
    try:
        servo_url = f"{ESP32_IP}/servo"  # Adjust to ESP32's endpoint
        response = requests.post(servo_url, json={"waste_type": class_label}, timeout=5)
        
        if response.status_code == 200:
            print(f"Servo command sent successfully: {class_label}")
        else:
            print(f" Failed to send servo command: {response.status_code} - {response.text}")
    except requests.exceptions.RequestException as e:
        print(f" Error sending servo command: {e}")

@app.route('/upload', methods=['POST'])
def upload_image():
    """Receives an image from ESP32-CAM, processes it, and classifies the waste."""
    if request.data:
        filename = os.path.join(UPLOAD_FOLDER, 'received_image.jpg')

        # Save the new image and overwrite the old one
        with open(filename, 'wb') as f:
            f.write(request.data)
            f.flush()
            os.fsync(f.fileno())  # Force write to disk
        
        print("New image received and saved.")

        # Introduce a small delay to ensure file write is completed
        time.sleep(1)

        # Preprocess the latest image
        img = preprocess_image(filename)
        if img is None:
            return jsonify({"error": "Failed to process image"}), 500

        # Make prediction
        predictions = model.predict(img)
        class_index = np.argmax(predictions)
        class_label = CLASS_LABELS[class_index]
        confidence = float(predictions[0][class_index])

        print(f"Predicted: {class_label} ({confidence:.2f})")

        # Control servo motor based on classification
        control_servo(class_label)

        return jsonify({"class": class_label, "confidence": confidence}), 200

    return jsonify({"error": "No image found"}), 400

@app.route('/servo', methods=['POST'])
def receive_servo_command():
    """Receives a servo control request from ESP32."""
    try:
        data = request.get_json()
        if not data or "waste_type" not in data:
            return jsonify({"error": "Invalid request format"}), 400
        
        waste_type = data["waste_type"]
        print(f"Received request to move servo for: {waste_type}")
        
        return jsonify({"status": "success", "waste_type": waste_type}), 200
    except Exception as e:
        print(f"Error processing servo request: {e}")
        return jsonify({"error": "Server error"}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
