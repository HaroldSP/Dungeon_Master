#include "led_strip.h"
#include <Adafruit_NeoPixel.h>

// Pin D2 on NodeMCU v2 = GPIO4
static const uint8_t kLedPin = 4;
// Default number of pixels if not specified
static uint16_t gNumPixels = 8;

static Adafruit_NeoPixel gStrip;

// Internal state for breathing effect
static uint16_t gPhase = 0;            // 0..1023
static unsigned long gLastUpdate = 0;
static const uint16_t kPhaseMax = 1024;
static const uint16_t kUpdateIntervalMs = 8; // ~125 FPS for smoother animation

// Smooth pulse using a sine-like curve via quadratic easing approximation
static uint8_t easedBrightness(uint16_t phase) {
  // Map phase 0..1023 -> 0..255 following a gentle in-out curve
  // Use a triangular wave folded then apply simple gamma correction
  uint16_t tri = phase <= 512 ? phase : (1024 - phase); // 0..512..0
  uint32_t scaled = (uint32_t)tri * 255 / 512;         // 0..255..0
  // Gamma-ish: y = (x/255)^2 * 255
  uint32_t y = (scaled * scaled) / 255;                // 0..255
  return (uint8_t)y;
}

void ledStripSetup(uint16_t numPixels) {
  gNumPixels = (numPixels > 0) ? numPixels : gNumPixels;
  gStrip.updateLength(gNumPixels);
  gStrip.setPin(kLedPin);
  gStrip.updateType(NEO_GRB + NEO_KHZ800); // WS2812/WS2813 timing
  gStrip.begin();
  gStrip.show(); // Initialize all pixels to 'off'
  gStrip.setBrightness(32); // Safer default brightness for single 18650 setups
}

void ledStripLoop() {
  const unsigned long now = millis();
  if (now - gLastUpdate < kUpdateIntervalMs) return;
  gLastUpdate = now;

  // Advance phase
  gPhase = (gPhase + 2) % kPhaseMax; // ~2x slower than original smoother setting
  const uint8_t b = easedBrightness(gPhase);

  // Purple base: mix red and blue; scale by breathing brightness
  // Base color roughly (R=160, G=0, B=255) scaled by b/255
  const uint8_t r = (uint16_t)(160 * b) / 255;
  const uint8_t g = 0;
  const uint8_t bl = (uint16_t)(255 * b) / 255;

  for (uint16_t i = 0; i < gNumPixels; ++i) {
    gStrip.setPixelColor(i, gStrip.Color(r, g, bl));
  }
  gStrip.show();
}


