#pragma once
#include <Arduino.h>

// Initializes the WS2813 LED strip connected to D2 (GPIO4)
void ledStripSetup(uint16_t numPixels);

// Runs the breathing purple effect; call frequently from loop()
void ledStripLoop();

// Immediately turns all LEDs off and shows the change
void ledStripAllOff();

// Modes
enum LedStripMode {
  LED_MODE_GLOW = 0,
  LED_MODE_CHASE = 1,
  LED_MODE_SOLID = 2,
};

// Set and get current mode
void ledStripSetMode(LedStripMode mode);
LedStripMode ledStripGetMode();

// Blink 3 times fast (for result indication)
void ledStripBlink3();

// Set all LEDs to solid purple (for constant on state)
void ledStripSolidOn();


