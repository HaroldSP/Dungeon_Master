# Dice Detection Server - Quick Setup

## The Problem

Edge Impulse on ESP32-CAM gives poor results for d20 dice (20 classes is too hard for embedded).

## The Solution

Offload detection to a PC/server running TensorFlow. ESP32 sends images → Server detects → Returns result.

---

## Quick Start (3 Steps)

### 1. Get Reference Repo (First Time Only)

```bash
git clone https://github.com/rsandrini/dnd-dice-detection-cnn.git reference_repo_1
```

This includes a pre-trained model at `reference_repo_1/dices_model.h5`

### 2. Install & Run Server

```bash
pip install -r dice_detection_server_requirements.txt
python dice_detection_server.py
```

Server runs on `http://0.0.0.0:5000`

### 3. Configure ESP32-CAM

Find your PC's IP (`ipconfig` on Windows, `ifconfig` on Linux/Mac), then connect to the tower page (`http://192.168.4.1/` or its STA IP).

In the new **External Detection Server** card:

1. Enter `http://YOUR_PC_IP:5000/detect`
2. Click **Save Server URL**
3. Hit **Capture & Recognize Dice**

Done! Call `/dice/capture` on ESP32 - it uses the server automatically.

---

## Training Your Own Model

1. **Organize images:**

   ```
   reference_repo_1/dataset/number_detection/train/d20/
   ├── 1/    (all images showing "1")
   ├── 2/    (all images showing "2")
   ├── ...
   └── 20/   (all images showing "20")
   ```

2. **Train:**

   ```bash
   cd reference_repo_1
   python train_model.py
   ```

3. **Copy model:**
   ```bash
   cp reference_repo_1/dices_model.h5 .
   ```

---

## Model Location

Server looks for model in this order:

1. `dices_model.h5` (root)
2. `models/dices_model.h5`
3. `reference_repo_1/dices_model.h5` ← Pre-trained (already exists!)

---

## Setup on New PC

After cloning this repo:

```bash
git clone https://github.com/rsandrini/dnd-dice-detection-cnn.git reference_repo_1
pip install -r dice_detection_server_requirements.txt
python dice_detection_server.py
```

---

## Files

- `dice_detection_server.py` - Flask server (auto-loads model)
- `src/esp32cam/` - ESP32 firmware (updated to use server)
- `reference_repo_1/` - Training scripts + pre-trained model (not in git, clone separately)
