#include "dice_detection.h"
#include <string.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Placeholder implementation. Detection will be reintroduced once the embedded
// ML pipeline is ready.
bool detectDiceFromRGB(const uint8_t* rgb, int width, int height, DiceDetection& out) {
  (void)rgb;
  (void)width;
  (void)height;
  memset(&out, 0, sizeof(out));
  out.detected = false;
  out.value = 0;
  out.confidence = 0.0f;
  return false;
}

// External server detection - sends JPEG to server and parses response
bool detectDiceFromJPEG(const uint8_t* jpeg, size_t jpegLen, const String& serverUrl, DiceDetection& out) {
  if (serverUrl.length() == 0 || WiFi.status() != WL_CONNECTED) {
    return false;
  }

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "image/jpeg");
  http.setTimeout(5000);  // 5 second timeout

  int httpCode = http.POST(jpeg, jpegLen);
  
  bool success = false;
  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
    String response = http.getString();
    
    // Parse JSON response: {"detected": true, "value": 15, "confidence": 0.95, "x": 100, "y": 50, "w": 80, "h": 80}
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    if (!err) {
      out.detected = doc["detected"] | false;
      out.value = doc["value"] | 0;
      out.confidence = doc["confidence"] | 0.0f;
      out.x = doc["x"] | 0;
      out.y = doc["y"] | 0;
      out.w = doc["w"] | 0;
      out.h = doc["h"] | 0;
      success = out.detected;
    }
  }
  
  http.end();
  return success;
}


