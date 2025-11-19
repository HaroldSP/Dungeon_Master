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
};

// Placeholder detection hook. Returns false until the ML model is integrated.
bool detectDiceFromRGB(const uint8_t* rgb, int width, int height, DiceDetection& out);

