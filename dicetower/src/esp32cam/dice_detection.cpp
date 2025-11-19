#include "dice_detection.h"
#include <string.h>

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


