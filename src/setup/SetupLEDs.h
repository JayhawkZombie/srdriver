#pragma once

#include <FastLED.h>
#include <Arduino.h>

/**
 * @brief The idea is that we can have any of the digital pins be used for LEDs, and be able
 * to use the same code to control them.
 * We can specify any portion of the global LED array to be used for a specific LED strip.
 */

// non-const version of pin_t
using pin_t = uint8_t;

bool InitLEDsOnPin(pin_t pin, int numLEDs, CRGB* leds, int startLED, int endLED);

class LEDLine
{
    private:
        pin_t _pin;
        int _numLEDs;
        CRGB* _leds;
        int _startLED;
        int _endLED;

    public:
        LEDLine(pin_t pin, int numLEDs, CRGB* leds, int startLED, int endLED)
        {
            _pin = pin;
            _leds = leds;
            _numLEDs = numLEDs;
            _startLED = startLED;
            _endLED = endLED;
        }
};
