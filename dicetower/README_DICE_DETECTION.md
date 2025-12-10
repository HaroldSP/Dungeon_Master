# ESP32-CAM Dice Detection (current state)

- Reference training repo and local TensorFlow model have been removed.
- Available detection: **ChatGPT Vision** via `dice_detection_server.py` `/gpt_detect`.
- Model-based `/detect` is disabled.

Quick start

1. Install deps: `pip install -r dice_detection_server_requirements.txt`
2. Run server: `python dice_detection_server.py` (port 5000)
3. In ESP32 UI: set detection server URL to `http://<pc-ip>:5000/detect`
4. Use **ChatGPT Guess** in the UI; it posts to `/gpt_detect`.

Files

- `dice_detection_server.py` - ChatGPT-only server
- `gpt_dice_integration.py` - ChatGPT Vision helper
- `src/esp32cam/` - ESP32 firmware and UI
