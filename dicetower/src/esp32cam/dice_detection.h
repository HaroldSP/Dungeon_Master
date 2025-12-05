#pragma once
#include <Arduino.h>

struct DiceDetection {
  bool detected;
  int value;
  int x;
  int y;
  int w;
  int h;
  float confidence;
  int second_most_likely;  // Second most likely value (for ChatGPT)
};

// Placeholder detection hook. Returns false until the ML model is integrated.
bool detectDiceFromRGB(const uint8_t* rgb, int width, int height, DiceDetection& out);

// External server detection (sends JPEG to server, returns result)
bool detectDiceFromJPEG(const uint8_t* jpeg, size_t jpegLen, const String& serverUrl, DiceDetection& out);

