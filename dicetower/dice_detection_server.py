#!/usr/bin/env python3
"""
Dice Detection Server for ESP32-CAM
Based on approaches from:
- https://github.com/rsandrini/dnd-dice-detection-cnn
- https://github.com/nell-byler/dice_detection

This server receives JPEG images from ESP32-CAM and returns dice detection results.
You can use either:
1. A pre-trained TensorFlow/Keras model (like the reference repos)
2. Computer vision (OpenCV) for simple cases
3. A hybrid approach

For d20 dice, this is much more reliable than running on ESP32 directly.
"""

from flask import Flask, request, jsonify
from flask_cors import CORS
import numpy as np
from PIL import Image
import io
import cv2
import base64
import os
import json

app = Flask(__name__)
CORS(app)  # Allow ESP32 to call this from any origin

# Optional ChatGPT integration is kept in a separate module so you can
# enable/disable or tweak prompts without touching this main server.
try:
    from gpt_dice_integration import detect_with_chatgpt
except Exception:
    detect_with_chatgpt = None

# Load trained model (based on reference_repo_1 approach)
model = None
model_loaded = False
class_mapping = None  # Will store class_index -> dice_value mapping

def load_model():
    """Load the dice classification model from reference repo."""
    global model, model_loaded, class_mapping
    
    if model_loaded:
        return model
    
    # Try multiple possible locations
    model_paths = [
        'dices_model.h5',  # Current directory
        'models/dices_model.h5',  # Models subdirectory
        'reference_repo_1/dices_model.h5',  # Reference repo
    ]
    
    model_path = None
    for path in model_paths:
        if os.path.exists(path):
            model_path = path
            break
    
    if model_path:
        try:
            from tensorflow import keras
            model = keras.models.load_model(model_path)
            model_loaded = True
            print(f"✓ Model loaded from: {model_path}")
            
            # Try to load class mapping
            mapping_paths = [
                'class_mapping.json',
                'reference_repo_1/class_mapping.json',
            ]
            for mp in mapping_paths:
                if os.path.exists(mp):
                    try:
                        with open(mp, 'r') as f:
                            mapping_data = json.load(f)
                            class_mapping = mapping_data.get('class_to_dice_value', {})
                            # Convert string keys to int
                            class_mapping = {int(k): int(v) for k, v in class_mapping.items()}
                            print(f"✓ Class mapping loaded from: {mp}")
                            print(f"  Mapping: {class_mapping}")
                            break
                    except Exception as e:
                        print(f"⚠ Failed to load mapping from {mp}: {e}")
            
            if class_mapping is None:
                print("⚠ No class_mapping.json found. Using default mapping (class_index + 1).")
                print("  This may be incorrect! Train with train_model_fixed.py to generate mapping.")
            
            return model
        except Exception as e:
            print(f"✗ Failed to load model from {model_path}: {e}")
    else:
        print("⚠ No model found. Detection will return placeholder results.")
        print("  Place your trained model as 'dices_model.h5' in the current directory")
        print("  Or train one using: cd reference_repo_1 && python train_model_fixed.py")
    
    return None

# Try to load model on startup
load_model()

def detect_dice_opencv(image_bytes):
    """
    Simple OpenCV-based detection (works for d6, harder for d20).
    This is a placeholder - you'll need to adapt for d20.
    """
    # Convert bytes to numpy array
    nparr = np.frombuffer(image_bytes, np.uint8)
    img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
    
    if img is None:
        return None
    
    # Convert to grayscale
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    
    # Simple threshold to find dice
    _, thresh = cv2.threshold(gray, 127, 255, cv2.THRESH_BINARY)
    
    # Find contours
    contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    
    # Find largest contour (assumed to be the dice)
    if not contours:
        return None
    
    largest = max(contours, key=cv2.contourArea)
    x, y, w, h = cv2.boundingRect(largest)
    
    # Crop to dice region
    dice_roi = gray[y:y+h, x:x+w]
    
    # For d20, you'd need a trained model here
    # For now, return a placeholder
    return {
        "detected": True,
        "value": 0,  # TODO: Implement actual detection
        "confidence": 0.5,
        "x": int(x),
        "y": int(y),
        "w": int(w),
        "h": int(h)
    }

def detect_dice_model(image_bytes):
    """
    Use a trained TensorFlow/Keras model (based on reference_repo_1).
    Model expects 480x480 images and outputs 20 classes (0-19 = dice values 1-20).
    """
    global model
    
    if model is None:
        load_model()
        if model is None:
            return {
                "detected": False,
                "value": 0,
                "confidence": 0.0,
                "x": 0,
                "y": 0,
                "w": 0,
                "h": 0
            }
    
    try:
        # Convert bytes to PIL Image
        img = Image.open(io.BytesIO(image_bytes))
        # Debug: log incoming resolution to verify ESP32 camera settings
        try:
            print("Incoming image size:", img.size)
        except Exception:
            pass
        
        # Convert to RGB if needed
        if img.mode != 'RGB':
            img = img.convert('RGB')
        
        # Derive expected size from model input to avoid hardcoding
        target_h, target_w = 480, 480
        try:
            if hasattr(model, "input_shape") and model.input_shape:
                # input_shape: (None, H, W, C)
                ish = model.input_shape
                if len(ish) == 4 and ish[1] and ish[2]:
                    target_h, target_w = int(ish[1]), int(ish[2])
        except Exception:
            pass
        img = img.resize((target_w, target_h))
        
        # Convert to numpy array and normalize (0-1 range)
        img_array = np.array(img) / 255.0
        
        # Add batch dimension (model expects shape: [1, H, W, 3])
        img_array = np.expand_dims(img_array, axis=0)
        
        # Run inference
        predictions = model.predict(img_array, verbose=0)
        
        # Get predicted class (0-19) and confidence
        predicted_class = int(np.argmax(predictions[0]))
        confidence = float(predictions[0][predicted_class])
        
        # Map class index to dice value using mapping if available
        if class_mapping and predicted_class in class_mapping:
            value = class_mapping[predicted_class]
        else:
            # Fallback: assume class_index + 1 = dice_value (may be incorrect!)
            value = predicted_class + 1
        
        return {
            "detected": confidence > 0.3,  # Threshold - adjust as needed
            "value": int(value),
            "confidence": confidence,
            "x": 0,  # Classification model doesn't provide bounding box
            "y": 0,
            "w": 0,
            "h": 0
        }
        
    except Exception as e:
        print(f"Detection error: {e}")
        return {
            "detected": False,
            "value": 0,
            "confidence": 0.0,
            "x": 0,
            "y": 0,
            "w": 0,
            "h": 0
        }

@app.route('/detect', methods=['POST'])
def detect():
    """
    Main detection endpoint.
    Receives JPEG image from ESP32-CAM and returns detection result.
    """
    # ESP32-CAM sends a raw JPEG buffer with Content-Type: image/jpeg
    # We keep this simple and always read raw bytes; if someone wants to
    # send multipart/form-data they still can (request.data will contain it).
    image_bytes = request.data or b""
    if not image_bytes:
        return jsonify({"error": "Empty image"}), 400
    
    try:
        # Use trained model (recommended for d20)
        result = detect_dice_model(image_bytes)
        
        # Fallback to OpenCV if model not available (not recommended for d20)
        if not result.get("detected") and model is None:
            result = detect_dice_opencv(image_bytes)
        
        if result is None:
            return jsonify({
                "detected": False,
                "value": 0,
                "confidence": 0.0,
                "x": 0,
                "y": 0,
                "w": 0,
                "h": 0
            }), 200
        
        return jsonify(result), 200
        
    except Exception as e:
        print(f"Detection error: {e}")
        return jsonify({"error": str(e)}), 500

@app.route('/health', methods=['GET'])
def health():
    """Health check endpoint."""
    return jsonify({"status": "ok"}), 200

@app.route('/gpt_detect', methods=['POST'])
def gpt_detect():
    """
    Alternate detection endpoint that uses ChatGPT (OpenAI) vision.
    ESP32-CAM should POST a JPEG image in the request body.
    """
    if detect_with_chatgpt is None:
        return jsonify({
            "detected": False,
            "value": 0,
            "confidence": 0.0,
            "x": 0,
            "y": 0,
            "w": 0,
            "h": 0,
            "error": "gpt_dice_integration not available"
        }), 500

    image_bytes = request.data or b""
    if not image_bytes:
        print("GPT DETECT: empty body from", request.remote_addr)
        return jsonify({"error": "Empty image"}), 400

    # Debug logging so you can see what is being sent to ChatGPT
    print(
        f"GPT DETECT: from {request.remote_addr}, "
        f"content_type={request.content_type}, "
        f"bytes={len(image_bytes)}"
    )

    result = detect_with_chatgpt(image_bytes)

    # Log model output for inspection
    try:
        print("GPT DETECT result:", result)
    except Exception:
        pass

    return jsonify(result), 200

if __name__ == '__main__':
    print("Starting Dice Detection Server...")
    print("ESP32-CAM should POST JPEG images to: http://<this-server-ip>:5000/detect")
    print("Example: POST /detection/server with JSON: {\"server_url\": \"http://192.168.1.100:5000/detect\"}")
    app.run(host='0.0.0.0', port=5000, debug=True)

