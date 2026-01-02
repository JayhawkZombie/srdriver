#pragma once

#include "PlatformConfig.h"

#if SUPPORTS_LEDS
#include <FastLED.h>
#include "Globals.h"

// Forward declaration for RGBW support
#if FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED
class Rgbw;
template<typename T1, typename T2> class RGBWEmulatedController;
#endif

/**
 * LEDStorage - Centralized storage for LED hardware arrays
 * 
 * This module contains all FastLED-related array definitions that were
 * previously scattered in main.cpp. This makes it easier to conditionally
 * compile LED support and keeps main.cpp clean.
 */

// FastLED array - this is what gets sent to hardware
extern CRGB leds[NUM_LEDS];

#if FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED
// RGBW support (forward declarations only in header)
extern Rgbw rgbw;
// rgbwEmu is defined in .cpp file and accessed via function
#endif

/**
 * Initialize FastLED hardware
 * This sets up the LED strip and blacks out all LEDs
 * Returns true if initialization successful, false otherwise
 */
bool initializeFastLED();

#endif // SUPPORTS_LEDS

