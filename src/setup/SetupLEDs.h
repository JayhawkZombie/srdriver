#pragma once

#include <FastLED.h>
#include <Arduino.h>

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
