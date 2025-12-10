#include "dice_detection.h"
#include <string.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Suppress deprecation warning for DynamicJsonDocument - we need fixed-size allocation
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

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
  out.second_most_likely = 0;
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

  int httpCode = http.POST(const_cast<uint8_t*>(jpeg), jpegLen);
  
  bool success = false;
  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
    String response = http.getString();
    
    DynamicJsonDocument doc(512);
    DeserializationError err = deserializeJson(doc, response);
    if (!err) {
      out.detected = doc["detected"] | false;
      out.value = doc["value"] | 0;
      out.confidence = doc["confidence"] | 0.0f;
      out.second_most_likely = doc["second_most_likely"] | 0;
      out.x = 0;
      out.y = 0;
      out.w = 0;
      out.h = 0;
      if (out.second_most_likely < 1 || out.second_most_likely > 20) {
        out.second_most_likely = 0;
      }
      success = out.detected;
    }
  }
  
  http.end();
  return success;
}

// Multipart/form-data post to fixed test server (field name: file)
bool detectDiceToTestServer(const uint8_t* jpeg, size_t jpegLen, const String& serverUrl, DiceDetection& out) {
  if (serverUrl.length() == 0 || WiFi.status() != WL_CONNECTED) {
    return false;
  }

  HTTPClient http;
  String boundary = "----esp32boundary";
  String contentType = "multipart/form-data; boundary=" + boundary;
  if (!http.begin(serverUrl)) {
    return false;
  }
  http.addHeader("Content-Type", contentType);
  http.setTimeout(8000);

  String head = "--" + boundary + "\r\n"
                "Content-Disposition: form-data; name=\"file\"; filename=\"capture.jpg\"\r\n"
                "Content-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--" + boundary + "--\r\n";

  size_t totalLen = head.length() + jpegLen + tail.length();
  uint8_t* body = (uint8_t*)malloc(totalLen);
  if (!body) {
    http.end();
    return false;
  }

  memcpy(body, head.c_str(), head.length());
  memcpy(body + head.length(), jpeg, jpegLen);
  memcpy(body + head.length() + jpegLen, tail.c_str(), tail.length());

  int httpCode = http.POST(body, totalLen);
  free(body);

  bool success = false;
  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
    String response = http.getString();
    DynamicJsonDocument doc(512);
    DeserializationError err = deserializeJson(doc, response);
    if (!err) {
      out.detected = doc["detected"] | false;
      out.value = doc["value"] | 0;
      out.confidence = doc["confidence"] | 0.0f;
      out.second_most_likely = doc["second_most_likely"] | 0;
      out.x = 0;
      out.y = 0;
      out.w = 0;
      out.h = 0;
      if (out.second_most_likely < 1 || out.second_most_likely > 20) {
        out.second_most_likely = 0;
      }
      success = out.detected;
    }
  }

  http.end();
  return success;
}
#pragma GCC diagnostic pop

