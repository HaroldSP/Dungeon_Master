# Dungeon Master Dice Tower Firmware (PlatformIO)

This firmware targets a NodeMCU V3 (ESP8266, CH340) and blinks the onboard LED (no LED strip required).

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
5. Open the Serial Monitor at 115200 baud. You should see `Starting onboard LED blink...` and the onboard LED should blink at 1 Hz.

## Notes

- The ESP8266 onboard LED is active-low: `LOW` turns it on, `HIGH` turns it off.
