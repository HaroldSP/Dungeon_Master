# ESP32-CAM Dice Detection - Quick Setup

## Hardware

- **ESP32-CAM** (AI-Thinker) with USB-to-Serial adapter
- **Power:** 5V via USB or external (stable power required)
- **Programming:** Connect GPIO0 to GND, press RESET, upload via PlatformIO

## The Problem

Edge Impulse on ESP32-CAM gives poor results for d20 dice (20 classes is too hard for embedded).

## The Solution

Offload detection to a PC/server. ESP32 sends images → Server detects → Returns result.

**Two detection methods:**

1. **TensorFlow model** (local server) - Fast, free, requires training
2. **ChatGPT Vision** (OpenAI API) - No training, costs per request

---

## Quick Start (3 Steps)

### 1. Get Reference Repo (First Time Only)

```bash
git clone https://github.com/rsandrini/dnd-dice-detection-cnn.git reference_repo_1
```

Pre-trained model: `reference_repo_1/dices_model.h5`

### 2. Install & Run Server

```bash
pip install -r dice_detection_server_requirements.txt
python dice_detection_server.py
```

Server runs on `http://0.0.0.0:5000`

### 3. Configure ESP32-CAM

1. Flash firmware: `pio run -e esp32cam -t upload`
2. Connect to ESP32 AP (`DiceTower-XXYY`, password: `dungeon123`) or its Wi-Fi IP
3. Open `http://192.168.4.1/` or `http://ESP32_IP/`
4. In **External Detection Server** card:
   - Enter `http://YOUR_PC_IP:5000/detect`
   - Click **Save Server URL**
5. Hit **Capture & Recognize Dice** or **ChatGPT Guess**

Done! ESP32 automatically uses the server.

---

## Detection Methods

### TensorFlow Model (Default)

- **Endpoint:** `/detect` (server)
- **Cost:** Free (local)
- **Speed:** ~100-250ms
- **Accuracy:** Depends on training data

### ChatGPT Vision (Optional)

- **Endpoint:** `/gpt_detect` (server)
- **Cost:** ~$0.01 per image (OpenAI API)
- **Speed:** ~1-3 seconds
- **Accuracy:** Good, no training needed

**Setup ChatGPT:**

1. Create `.env` file: `OPENAI_API_KEY=sk-...`
2. Server auto-loads key from `.env`
3. Use **ChatGPT Guess** button in ESP32 UI

---

## Training Your Own Model

1. **Organize images:**

   ```
   reference_repo_1/dataset/number_detection/train/d20/
   ├── 1/    (all images showing "1")
   ├── 2/    (all images showing "2")
   └── ...   (folders 1-20)
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

## Files

- `dice_detection_server.py` - Flask server (TensorFlow + ChatGPT)
- `gpt_dice_integration.py` - ChatGPT Vision integration
- `src/esp32cam/` - ESP32 firmware
- `reference_repo_1/` - Training scripts + model (clone separately)

---

## Setup on New PC

```bash
git clone <this-repo>
git clone https://github.com/rsandrini/dnd-dice-detection-cnn.git reference_repo_1
pip install -r dice_detection_server_requirements.txt
python dice_detection_server.py
```
