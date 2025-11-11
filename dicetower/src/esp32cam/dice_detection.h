#pragma once
#include <Arduino.h>

struct DiceDetection {
  bool detected;
  int value;
  int x;
  int y;
  int w;
  int h;
};

extern const uint8_t kDigitFont5x7[10][7];

// Detect dice pips and digits from an RGB888 frame.
// rgb: width*height*3 buffer (BGR or RGB is fine if you pass channels in order used).
// Returns true if a plausible face (pips or digit) was found.
bool detectDiceFromRGB(const uint8_t* rgb, int width, int height, DiceDetection& out);

