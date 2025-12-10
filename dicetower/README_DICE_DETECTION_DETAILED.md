# Dice Detection Notes (current)

- Reference training repo and local TensorFlow model were removed.
- Detection available: ChatGPT Vision via `dice_detection_server.py` at `/gpt_detect`.
- `/detect` is a placeholder (disabled).

Usage

1. `pip install -r dice_detection_server_requirements.txt`
2. `python dice_detection_server.py`
3. In ESP UI set server URL to `http://<pc-ip>:5000/detect`
4. Use “ChatGPT Guess” button (hits `/gpt_detect`).

Files

- `dice_detection_server.py` – ChatGPT-only server
- `gpt_dice_integration.py` – ChatGPT Vision helper
- `src/esp32cam/` – ESP32 firmware/UI
