#pragma once
#include <Arduino.h>

// Initializes the WS2813 LED strip connected to D2 (GPIO4)
void ledStripSetup(uint16_t numPixels);

// Runs the breathing purple effect; call frequently from loop()
void ledStripLoop();


