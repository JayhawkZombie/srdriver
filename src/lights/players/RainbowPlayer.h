#pragma once

#include <FastLED.h>
#include "../Light.h"

/**
 * @brief Run through the RGB spectrum, using HSV
 * and full saturation.
 * 
 */
class RainbowPlayer
{
    Light* _leds;
    int _numLEDs;
    int _startLED;
    int _endLED;
    float _speed;
    uint8_t _currentHue;
    bool _reverseDirection;
    bool _enabled;  // New: enable/disable flag

public:
    RainbowPlayer(Light* leds, int numLEDs, int startLED, int endLED, float speed = 1.0f, bool reverseDirection = false);
    void update(float dtSeconds);
    void setSpeed(float speed);
    void setHue(uint8_t hue);
    void setDirection(bool reverseDirection);
    void setLEDs(Light* leds);
    void setNumLEDs(int numLEDs);
    void setStartLED(int startLED);
    void setEndLED(int endLED);
    void setEnabled(bool enabled);  // New: enable/disable method
    bool isEnabled() const;  // New: check if enabled
};
