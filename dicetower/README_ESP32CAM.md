# ESP32-CAM Dice Recognition Firmware

This firmware adds dice recognition capabilities to the Dice Tower project using an ESP32-CAM module.

## Hardware Requirements

- **ESP32-CAM** (AI-Thinker module recommended)
- **USB-to-Serial adapter** (e.g., ESP32-CAM-MB, FTDI, or CP2102) for programming
- **Power supply**: 5V via USB or external power (ensure stable power for camera)
- **MicroSD card** (optional, for storing images)

## Wiring for Programming

When programming the ESP32-CAM, you need to:

1. **Connect GPIO0 to GND** (puts ESP32 into download mode)
2. **Press and hold RESET button**
3. **Release RESET** while keeping GPIO0 grounded
4. Upload firmware via PlatformIO
5. **Disconnect GPIO0 from GND** and press RESET to run

Alternatively, use an ESP32-CAM-MB board which handles this automatically.

## PlatformIO Setup

1. Open the `dicetower` folder in VS Code with PlatformIO extension
2. Select the **`esp32cam`** environment from the PlatformIO toolbar
3. Build and upload:
   - Click "Upload" in PlatformIO
   - Or use terminal: `pio run -e esp32cam -t upload`

## Camera Configuration

The firmware is configured for the standard AI-Thinker ESP32-CAM pinout:
- Camera resolution: QVGA (320x240) for faster processing
- JPEG quality: 12 (balanced quality/speed)
- Uses PSRAM if available for better performance

## Wi-Fi Setup

Same provisioning system as ESP8266 version:

1. **First boot**: Device creates AP `DiceTower-XXYY` (password: `dungeon123`)
2. Connect your phone/PC to the AP
3. Open `http://192.168.4.1` in browser
4. Use the provisioning form to enter your Wi-Fi credentials
5. Device will connect to your network and provide its IP address

## HTTP API Endpoints

### Dice Recognition
- `GET /dice/capture` - Captures an image and attempts to recognize dice value
  - Returns: `{"ok":true, "detected":true/false, "value":1-6, "timestamp":ms}`
- `GET /dice/status` - Gets the last dice recognition result
  - Returns: `{"ok":true, "detected":true/false, "value":1-6, "timestamp":ms}`

### Status
- `GET /status` - Device status including camera and dice state
  - Returns: `{"camera":true/false, "dice":{...}}`

### Provisioning (same as ESP8266)
- `GET /provision?ssid=SSID&pass=PASSWORD` - Save Wi-Fi credentials
- `GET /wipe` - Erase credentials and reboot
- `GET /name?action=get` - Get tower name
- `GET /name?action=set&name=NAME` - Set tower name

## Dice Recognition Algorithm

**Current Status**: Placeholder implementation

The `recognizeDice()` function in `main.cpp` currently returns a placeholder value. To implement actual dice recognition, you'll need to:

1. **Convert JPEG to grayscale** (if needed)
2. **Apply image processing**:
   - Threshold to find dice
   - Edge detection or contour finding
   - Circle detection for pips
3. **Count pips** to determine dice value (1-6)
4. **Return the value**

### Recommended Approaches

1. **Simple Computer Vision**:
   - Use OpenCV-style algorithms (threshold, findContours, HoughCircles)
   - Works well with good lighting and consistent dice

2. **Machine Learning**:
   - Train a lightweight model (TensorFlow Lite Micro)
   - More robust but requires more memory/flash
   - Can use Edge Impulse for training

3. **Template Matching**:
   - Pre-defined templates for each dice face
   - Fast but less flexible

## Frontend Integration

The frontend (`ManageTowers.vue`) has been updated with:
- **"Roll Dice" button** for each ESP32-CAM tower
- **Dice result display** showing the recognized value
- **Timestamp** of the last roll

## Troubleshooting

### Camera not initializing
- Check power supply (camera needs stable 5V)
- Verify pin connections match your ESP32-CAM module
- Check Serial Monitor for error codes

### Out of memory errors
- Reduce camera resolution in `initCamera()`
- Reduce JPEG quality
- Ensure PSRAM is enabled (should be automatic)

### Upload fails
- Ensure GPIO0 is grounded during upload
- Try lower upload speed in `platformio.ini`
- Check USB-to-Serial driver installation

### Dice recognition not working
- Implement the actual recognition algorithm (currently placeholder)
- Ensure good lighting
- Position dice clearly in camera view
- Adjust camera focus if your module has a focus ring

## Next Steps

1. **Implement dice recognition algorithm** in `recognizeDice()` function
2. **Test with real dice** and adjust parameters
3. **Optimize for speed** if needed (lower resolution, faster processing)
4. **Add multiple dice support** if desired
5. **Add image streaming** for debugging (optional `/camera/stream` endpoint)

## Notes

- The ESP32-CAM uses different Wi-Fi libraries than ESP8266 (WiFi.h vs ESP8266WiFi.h)
- Camera initialization takes a few seconds on first boot
- Keep camera lens clean for best results
- Consider adding a physical button to trigger dice capture

