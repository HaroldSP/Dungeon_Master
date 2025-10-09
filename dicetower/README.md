# Dungeon Master Dice Tower Firmware (PlatformIO)

This firmware targets a NodeMCU V3 (ESP8266, CH340) and blinks the onboard LED (no LED strip required). It starts a Wi‑Fi Access Point and exposes an HTTP API to control blinking.

## Hardware

- Controller: NodeMCU V3 (ESP8266, CH340 USB-serial)
- Power: USB cable from your PC.

## PlatformIO

Requirements: VS Code with PlatformIO extension installed.

1. Connect the NodeMCU via USB. On Windows, ensure the CH340 driver is installed. A COM port should appear (e.g., `COM5`).
2. Open this folder (`dicetower`) in VS Code.
3. Inspect `platformio.ini` to confirm `board = nodemcuv2`.
4. Build and upload:
   - In PlatformIO, select the `nodemcuv2` environment.
   - Click “Upload”. If upload fails, try a lower `upload_speed` in `platformio.ini`.
5. Open the Serial Monitor at 115200 baud. You should see AP credentials and IP printed. Connect your phone/PC to the AP.

## Wi‑Fi AP + HTTP API

- AP SSID: `DiceTower-XXYY` (last bytes from MAC)
- AP password: `dungeon123`
- Default IP: typically `192.168.4.1`

Endpoints (GET):

- `/` → text status
- `/status` → `{ blinking: boolean, ledOn: boolean }`
- `/blink/on` → enable blinking
- `/blink/off` → disable blinking (LED off)
- `/blink/toggle` → toggle blinking

## Notes

- The ESP8266 onboard LED is active-low: `LOW` turns it on, `HIGH` turns it off.
