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

## Wi‑Fi AP + HTTP API + Provisioning

- AP SSID: `DiceTower-XXYY` (last bytes from MAC)
- AP password: `dungeon123`
- Default IP: typically `192.168.4.1`

Endpoints (GET):

- `/` → text status + provisioning form
- `/status` → `{ blinking: boolean, ledOn: boolean }`
- `/blink/on` → enable blinking
- `/blink/off` → disable blinking (LED off)
- `/blink/toggle` → toggle blinking
- `/provision?ssid=YOUR_SSID&pass=YOUR_PASSWORD` → saves Wi‑Fi creds to flash and attempts to connect as STA. If successful, response includes `ip`.
- `/wipe` → deletes saved Wi‑Fi credentials and restarts. After reboot, the device should return to AP mode.
- `/ap/toggle?action=on|off` → toggle AP broadcasting (on/off). Without action, returns current AP status.
- `/name?action=get` → get stored tower name
- `/name?action=set&name=NAME` → set tower name

### Multi‑device workflow (5+ towers)

1. First power‑up → each tower starts its own AP: `DiceTower-XXYY`.
2. In the app (left sidebar), open a “Dice Towers” panel listing discovered/known devices. For first setup, connect your PC/phone to the tower AP.
3. Open the device page (or use the frontend control URL field) and call `/provision?ssid=...&pass=...`.
4. The tower saves creds, reboots/joins your Wi‑Fi, and prints its IP in the response. Switch your PC back to your Wi‑Fi and add that IP to the app’s list.
5. Repeat for each tower; assign friendly names in the app.

### Credentials storage and safety

- Device stores SSID/password in LittleFS (`/creds.json`) on the microcontroller, not in your git repo.
- Do NOT hardcode your Wi‑Fi in source. For local testing, use the `/provision` endpoint or a local untracked config.
- To avoid committing any local configs, add patterns to `.gitignore` (already not needed for device creds as they are on-device flash).

### Erase flash to guarantee AP on first boot

- PlatformIO task: Erase Flash. From terminal in this project: `pio run --target erase` then `pio run --target upload`.
- Or, after flashing, call `http://<device>/wipe` to remove creds and reboot into AP mode.

## Notes

- The ESP8266 onboard LED is active-low: `LOW` turns it on, `HIGH` turns it off.
