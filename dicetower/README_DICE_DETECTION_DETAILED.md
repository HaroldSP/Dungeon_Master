# Dice Detection Server - Detailed Guide

## The Problem

Edge Impulse on ESP32-CAM gives poor results for d20 dice detection. The feature explorer shows heavily overlapping clusters, meaning the model can't distinguish between the 20 different dice faces. This happens because:

- **20 classes is too complex** for embedded ML on ESP32-CAM
- **Limited memory/compute** on ESP32 restricts model size
- **Low resolution** camera (QVGA: 320x240) loses detail
- **Feature overlap** - different dice faces look too similar in reduced feature space

## The Solution

Offload detection to a PC/server running full TensorFlow. This approach:

- **ESP32-CAM** captures image (JPEG)
- **ESP32** sends JPEG to your PC/server via HTTP POST
- **Server** runs TensorFlow/Keras model (like the reference repos)
- **Server** returns detection result (value 1-20, confidence, bounding box)
- **ESP32** displays result

**Benefits:**

- More compute power (PC can run larger models)
- Better accuracy (full TensorFlow, not Edge Impulse constraints)
- Easier training (use reference repo approaches directly)
- Flexible (switch models without reflashing ESP32)

---

## Quick Start (3 Steps)

### 1. Get Reference Repo (First Time Only)

The reference repository contains a pre-trained model and training scripts:

```bash
git clone https://github.com/rsandrini/dnd-dice-detection-cnn.git reference_repo_1
```

**What this gives you:**

- Pre-trained model: `reference_repo_1/dices_model.h5` (~158 MB)
- Training script: `reference_repo_1/train_model.py`
- Example dataset structure: `reference_repo_1/dataset/`
- Prediction examples: `reference_repo_1/predict.py`, `predict-camera.py`

**Note:** This repo is large (518 MB) because it includes the model and dataset. It's not committed to git (see `.gitignore`).

### 2. Install & Run Server

**Install Python dependencies:**

```bash
pip install -r dice_detection_server_requirements.txt
```

**Required packages:**

- `flask` - Web server framework
- `flask-cors` - CORS support (allows ESP32 to call server)
- `tensorflow` - ML framework (for model inference)
- `opencv-python` - Image processing (optional, for fallback)
- `numpy` - Numerical operations
- `Pillow` - Image handling

**Run the server:**

```bash
python dice_detection_server.py
```

**Expected output:**

```
✓ Model loaded from: reference_repo_1/dices_model.h5
Starting Dice Detection Server...
ESP32-CAM should POST JPEG images to: http://<this-server-ip>:5000/detect
 * Running on http://0.0.0.0:5000
```

**Server endpoints:**

- `POST /detect` - Receives JPEG image, returns detection JSON
- `GET /health` - Health check (returns `{"status": "ok"}`)

**Troubleshooting:**

- **"No model found"**: Make sure `reference_repo_1/dices_model.h5` exists
- **Port already in use**: Change port in `dice_detection_server.py` (line with `app.run(port=5000)`)
- **Import errors**: Run `pip install -r dice_detection_server_requirements.txt` again

### 3. Configure ESP32-CAM

**Find your PC's IP address:**

- **Windows:** Open Command Prompt, run `ipconfig`, look for "IPv4 Address" under your Wi-Fi adapter
- **Linux/Mac:** Open Terminal, run `ifconfig` or `ip addr`, look for "inet" address

**Example:** Your PC might be `192.168.1.100`

**Configure ESP32-CAM via web interface:**

1. Connect to ESP32-CAM's AP or Wi-Fi
2. Open `http://ESP32_IP/` in browser
3. Use the web form to set detection server URL

**Or configure via API:**

```bash
curl -X POST http://ESP32_IP/detection/server \
  -H "Content-Type: application/json" \
  -d '{"server_url": "http://192.168.1.100:5000/detect"}'
```

**Replace:**

- `ESP32_IP` = Your ESP32-CAM's IP address
- `192.168.1.100` = Your PC's IP address

**Verify configuration:**

```bash
curl http://ESP32_IP/detection/server
```

Should return:

```json
{
  "ok": true,
  "server_url": "http://192.168.1.100:5000/detect",
  "enabled": true
}
```

**Network requirements:**

- ESP32-CAM and PC must be on **same Wi-Fi network**
- PC firewall must allow **port 5000** (incoming connections)
- Low latency helps (same room/network recommended)

**Firewall setup:**

- **Windows:** Windows Defender Firewall → Allow an app → Python → Check "Private"
- **Linux:** `sudo ufw allow 5000`
- **Mac:** System Preferences → Security & Privacy → Firewall → Options → Add Python

**Test connection:**

```bash
# From ESP32 or another device on same network
curl http://YOUR_PC_IP:5000/health
```

Should return: `{"status": "ok"}`

**Done!** Now when you call `/dice/capture` on ESP32, it automatically sends images to your server and gets detection results.

---

## Training Your Own Model

The pre-trained model works, but training on your own dice will give better accuracy for your specific setup.

### Step 1: Organize Images

**Create folder structure:**

```
reference_repo_1/dataset/number_detection/train/d20/
├── 1/     (all images showing dice face "1")
├── 2/     (all images showing dice face "2")
├── 3/     (all images showing dice face "3")
├── ...
└── 20/    (all images showing dice face "20")
```

**Image requirements:**

- **Format:** JPG or JPEG
- **Size:** Any size (will be resized to 480x480 during training)
- **Quality:** Clear, well-lit images work best
- **Quantity:** At least 50-100 images per class, more is better
  - Reference repo has 70-500+ images per class
  - More variety = better model (different angles, lighting, backgrounds)

**Tips for good training data:**

- Capture dice from multiple angles
- Vary lighting conditions (bright, dim, shadows)
- Include different backgrounds
- Show dice at different distances
- Include slightly blurry/off-angle images (realistic conditions)
- Balance classes (similar number of images per face)

**Using ESP32-CAM images:**

1. Capture images via `/dice/capture` endpoint
2. Save images locally
3. Manually label them (which face is showing)
4. Organize into folders

**Using Edge Impulse data:**

1. Export your dataset from Edge Impulse
2. Reorganize into the folder structure above
3. Use those images for training

### Step 2: Train the Model

**Navigate to reference repo:**

```bash
cd reference_repo_1
```

**Install training dependencies:**

```bash
pip install tensorflow keras pillow numpy
```

**Run training:**

```bash
python train_model.py
```

**What happens:**

- Loads images from `dataset/number_detection/train/d20/`
- Resizes to 480x480 pixels
- Applies data augmentation (rotation, zoom, flip)
- Trains CNN model (50 epochs by default)
- Saves model as `dices_model.h5`

**Training time:**

- Depends on number of images and your PC
- With 1000+ images: 10-30 minutes on modern CPU, 2-5 minutes on GPU
- Watch for overfitting (validation accuracy stops improving)

**Model architecture (from reference repo):**

- Input: 480x480 RGB images
- Conv2D layers: 32, 32, 64 filters
- MaxPooling after each conv layer
- Dense layer: 64 neurons
- Output: 20 classes (softmax activation)

**Training output:**

```
Epoch 1/50
...
Epoch 50/50
...
Model saved to dices_model.h5
```

**Troubleshooting:**

- **Out of memory:** Reduce batch size in `train_model.py` (line with `batch_size = 32`)
- **Slow training:** Use GPU if available, or reduce image count
- **Poor accuracy:** Add more training images, check image quality

### Step 3: Use Your Model

**Copy model to server location:**

```bash
# From reference_repo_1 directory
cp dices_model.h5 ../dices_model.h5

# Or from main directory
cp reference_repo_1/dices_model.h5 .
```

**Restart server:**

- Stop server (Ctrl+C)
- Start again: `python dice_detection_server.py`
- Should see: `✓ Model loaded from: dices_model.h5`

**Test your model:**

1. Capture image via ESP32: `GET /dice/capture`
2. Check result: `GET /dice/status`
3. Verify accuracy with known dice values

**Model comparison:**

- Pre-trained model: Works on reference repo's dice
- Your trained model: Optimized for your dice/lighting/angles

---

## Model Location

The server automatically searches for the model in this order:

1. **`dices_model.h5`** (root directory) - Recommended for your custom model
2. **`models/dices_model.h5`** (models subdirectory) - Alternative location
3. **`reference_repo_1/dices_model.h5`** (reference repo) - Pre-trained model (already exists!)

**First match wins!** The server prints which one it found:

```
✓ Model loaded from: reference_repo_1/dices_model.h5
```

**Custom model path:**
To use a different location, edit `dice_detection_server.py`:

```python
model_paths = [
    'dices_model.h5',
    'models/dices_model.h5',
    'reference_repo_1/dices_model.h5',
    'path/to/your/custom/model.h5',  # Add here
]
```

**Model specifications:**

- **Format:** Keras H5 file (`.h5` extension)
- **Input:** 480x480 RGB images (3 channels)
- **Output:** 20 classes (0-19, maps to dice values 1-20)
- **Framework:** TensorFlow/Keras
- **Size:** ~158 MB (pre-trained model)

---

## Setup on New PC

When you clone this repository on a new PC, the reference repos aren't included (they're in `.gitignore` because they're 500+ MB).

### Step 1: Clone This Repository

```bash
git clone <your-repo-url>
cd dicetower
```

### Step 2: Download Reference Repo

```bash
git clone https://github.com/rsandrini/dnd-dice-detection-cnn.git reference_repo_1
```

This downloads:

- Pre-trained model (`dices_model.h5`)
- Training scripts
- Example dataset structure

**Time:** 1-2 minutes (depends on internet speed, repo is 518 MB)

### Step 3: Install Dependencies

```bash
pip install -r dice_detection_server_requirements.txt
```

**Verify installation:**

```bash
python -c "import tensorflow; import flask; print('OK')"
```

Should print: `OK`

### Step 4: Verify Model

```bash
ls -lh reference_repo_1/dices_model.h5
```

Should show file exists (~158 MB)

### Step 5: Run Server

```bash
python dice_detection_server.py
```

Should see:

```
✓ Model loaded from: reference_repo_1/dices_model.h5
Starting Dice Detection Server...
```

**You're ready!** Follow step 3 of Quick Start to configure ESP32-CAM.

---

## How It Works

### ESP32-CAM Side

**Code changes made:**

- `src/esp32cam/main.cpp` - Added HTTP client, external server support
- `src/esp32cam/dice_detection.cpp` - Added `detectDiceFromJPEG()` function
- `src/esp32cam/dice_detection.h` - Added function declaration

**Flow:**

1. User calls `GET /dice/capture`
2. ESP32 captures JPEG from camera
3. If `externalDetectionServer` is configured:
   - Convert JPEG to HTTP POST request
   - Send to server URL
   - Wait for JSON response
   - Parse detection result
4. Return result to user

**API endpoints:**

- `GET /detection/server` - Get current server URL
- `POST /detection/server` - Set server URL: `{"server_url": "http://..."}`
- `GET /dice/capture` - Capture and detect (uses server if configured)
- `GET /dice/status` - Get last detection result

### Server Side

**Detection flow:**

1. Receive JPEG image via HTTP POST
2. Convert bytes to PIL Image
3. Resize to 480x480 (model input size)
4. Normalize pixel values (0-1 range)
5. Run model inference
6. Get predicted class (0-19)
7. Map to dice value (1-20)
8. Return JSON response

**Response format:**

```json
{
  "detected": true,
  "value": 15,
  "confidence": 0.92,
  "x": 0,
  "y": 0,
  "w": 0,
  "h": 0
}
```

**Classification vs Object Detection:**

- Current implementation: **Classification** (just the value, no bounding box)
- Reference repo 2: **Object Detection** (value + location)
- For d20, classification is simpler and often more accurate

### Network Communication

**Request (ESP32 → Server):**

```
POST /detect HTTP/1.1
Host: 192.168.1.100:5000
Content-Type: image/jpeg
Content-Length: 12345

[JPEG image bytes]
```

**Response (Server → ESP32):**

```
HTTP/1.1 200 OK
Content-Type: application/json

{"detected": true, "value": 15, "confidence": 0.92, ...}
```

**Latency:**

- Network: 10-50ms (same Wi-Fi network)
- Inference: 50-200ms (depends on PC/GPU)
- Total: ~100-250ms per detection

---

## Troubleshooting

### Server Issues

**"No model found" error:**

- Check `reference_repo_1/dices_model.h5` exists
- Or copy model: `cp reference_repo_1/dices_model.h5 .`
- Check file permissions

**Model loads but gives wrong results:**

- Pre-trained model was trained on specific dice
- Train your own model with your dice for better accuracy
- Check image quality (lighting, focus, angle)

**Server crashes:**

- Check Python version (3.8+ required)
- Reinstall dependencies: `pip install --upgrade -r dice_detection_server_requirements.txt`
- Check error logs in terminal

### ESP32 Issues

**Can't connect to server:**

- Verify server is running: `curl http://YOUR_PC_IP:5000/health`
- Check firewall allows port 5000
- Verify both devices on same Wi-Fi network
- Check ESP32's Wi-Fi connection: `GET /status`

**Detection always returns value 0:**

- Server not configured: Check `GET /detection/server`
- Server URL incorrect: Verify IP address and port
- Network issue: Check ESP32 can reach server IP

**Slow detection:**

- Normal: 100-250ms is expected
- Network latency: Use same Wi-Fi network, avoid VPN
- Server overload: Close other applications

### Model Issues

**Low confidence scores:**

- Model not trained on your dice/lighting
- Train your own model with your images
- Improve image quality (better lighting, focus)

**Wrong values detected:**

- Model confusion between similar faces
- Add more training data for confused classes
- Check if dice faces are clearly visible in images

**Training fails:**

- Out of memory: Reduce batch size
- No images found: Check folder structure matches expected format
- Import errors: Install TensorFlow: `pip install tensorflow`

---

## Advanced Configuration

### Custom Model Path

Edit `dice_detection_server.py`, function `load_model()`:

```python
model_paths = [
    'dices_model.h5',
    'models/dices_model.h5',
    'reference_repo_1/dices_model.h5',
    'custom/path/to/model.h5',  # Add your path
]
```

### Change Detection Threshold

Edit `dice_detection_server.py`, function `detect_dice_model()`:

```python
return {
    "detected": confidence > 0.3,  # Change 0.3 to your threshold (0.0-1.0)
    ...
}
```

Lower threshold (0.1) = more detections, more false positives
Higher threshold (0.7) = fewer detections, more accurate

### Change Server Port

Edit `dice_detection_server.py`, last line:

```python
app.run(host='0.0.0.0', port=5000, debug=True)  # Change 5000 to your port
```

Update ESP32 configuration with new port:

```json
{ "server_url": "http://YOUR_PC_IP:NEW_PORT/detect" }
```

### Use GPU for Faster Inference

If you have NVIDIA GPU:

```bash
pip install tensorflow-gpu
```

Server will automatically use GPU if available (much faster inference).

### Multiple ESP32-CAMs

One server can handle multiple ESP32-CAMs:

- Each ESP32 calls the same server URL
- Server processes requests sequentially
- No special configuration needed

For high throughput, consider:

- Using a more powerful server
- Running multiple server instances on different ports
- Using a load balancer

---

## Files Reference

| File                                     | Purpose                                 | Location                    |
| ---------------------------------------- | --------------------------------------- | --------------------------- |
| `dice_detection_server.py`               | Flask server with model inference       | Root                        |
| `dice_detection_server_requirements.txt` | Python dependencies                     | Root                        |
| `src/esp32cam/main.cpp`                  | ESP32 firmware (updated for server)     | `src/esp32cam/`             |
| `src/esp32cam/dice_detection.cpp`        | Detection functions                     | `src/esp32cam/`             |
| `src/esp32cam/dice_detection.h`          | Detection header                        | `src/esp32cam/`             |
| `reference_repo_1/`                      | Training scripts + pre-trained model    | External (clone separately) |
| `reference_repo_2/`                      | Alternative approach (object detection) | External (optional)         |

---

## Reference Repositories Explained

### Repo 1: rsandrini/dnd-dice-detection-cnn

**Approach:** Simple CNN classification

- **Input:** Single image (480x480)
- **Output:** Dice value (1-20)
- **Model:** Sequential CNN (Conv2D + Dense layers)
- **Pros:** Simple, fast, easy to train
- **Cons:** No bounding box (classification only)
- **Best for:** When dice is centered/visible in frame

**How it works:**

1. Images organized by class (folders 1-20)
2. CNN learns features for each face
3. Softmax output gives probability for each class
4. Highest probability = detected value

### Repo 2: nell-byler/dice_detection

**Approach:** Object Detection (SSD)

- **Input:** Full image
- **Output:** Bounding box + class
- **Model:** SSD MobileNet (TensorFlow Object Detection API)
- **Pros:** Finds dice location, handles multiple dice
- **Cons:** More complex, harder to train, larger model
- **Best for:** Multiple dice, dice not centered

**Note:** This repo is for d6 (6-sided dice), would need adaptation for d20.

---

## Next Steps

1. ✅ **Code is ready** - ESP32 can send images to server
2. ⏳ **Test with pre-trained model** - See if it works on your dice
3. ⏳ **Train your own model** - If pre-trained doesn't work well
4. ⏳ **Optimize** - Adjust threshold, improve images, etc.
5. ⏳ **Deploy** - Consider Raspberry Pi for lower latency

---

## FAQ

**Q: Do I need to train my own model?**  
A: Try the pre-trained model first. Train your own only if accuracy is poor on your dice.

**Q: Can I use Edge Impulse model on the server?**  
A: Yes! Export from Edge Impulse, convert to Keras format, load in server.

**Q: How accurate is this?**  
A: Depends on your model and images. Pre-trained: ~70-80% on reference dice. Your trained: 85-95% on your dice.

**Q: Can this run on Raspberry Pi?**  
A: Yes! Raspberry Pi 4 can run TensorFlow models. Lower latency than PC if placed near ESP32.

**Q: What if I have multiple dice?**  
A: Current implementation detects one dice. For multiple, use object detection approach (repo 2) or call detection multiple times.

**Q: How do I improve accuracy?**  
A: More training data, better lighting, consistent angles, train on your specific dice.
