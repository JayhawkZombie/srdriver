#include "RainbowPlayer.h"
#include <FastLED.h>

RainbowPlayer::RainbowPlayer(Light *leds, int numLEDs, int startLED, int endLED, float speed, bool reverseDirection)
    : _leds(leds)
    , _numLEDs(numLEDs)
    , _startLED(startLED)
    , _endLED(endLED)
    , _speed(speed)
    , _currentHue(0)
    , _reverseDirection(reverseDirection)
    , _enabled(true)  // Default to enabled
{
    // Validate parameters
    if (_endLED < _startLED)
    {
        _endLED = _startLED;
    }
    if (_endLED >= _numLEDs)
    {
        _endLED = _numLEDs - 1;
    }
    if (_startLED < 0)
    {
        _startLED = 0;
    }
}

void RainbowPlayer::update(float dtSeconds)
{
    if (!_leds || _numLEDs <= 0 || !_enabled)
    {
        return;  // Don't update if disabled
    }

    // Update the base hue based on time and speed
    // Speed is in rotations per second, so multiply by 255 to get hue units per second
    // Then multiply by dtSeconds to get the hue increment for this frame
    float hueIncrement = _speed * 255.0f * dtSeconds;
    _currentHue += (uint8_t) hueIncrement;

    // Use a fixed hue step like classic FastLED rainbow examples
    // This creates a flowing rainbow effect where each LED has a distinct color
    uint8_t hueStep = 5; // Classic FastLED rainbow step

    // Update each LED in the range
    for (int i = _startLED; i <= _endLED; i++)
    {
        if (i >= 0 && i < _numLEDs)
        {
            // Calculate hue for this LED
            // Each LED gets a hue offset based on its position, creating a flowing rainbow
            uint8_t ledHue;

            if (_reverseDirection)
            {
                // Reverse direction: rainbow flows from end to start
                ledHue = _currentHue + (_endLED - i) * hueStep;
            }
            else
            {
                // Normal direction: rainbow flows from start to end
                ledHue = _currentHue + (i - _startLED) * hueStep;
            }

            // Convert HSV to RGB using FastLED's direct assignment
            CRGB rgbColor = CHSV(ledHue, 255, 255); // Full saturation and value

            // Set the LED color
            _leds[i].r = rgbColor.r;
            _leds[i].g = rgbColor.g;
            _leds[i].b = rgbColor.b;
        }
    }
}

void RainbowPlayer::setSpeed(float speed)
{
    _speed = speed;
}

void RainbowPlayer::setHue(uint8_t hue)
{
    _currentHue = hue;
}

void RainbowPlayer::setDirection(bool reverseDirection)
{
    _reverseDirection = reverseDirection;
}

void RainbowPlayer::setEnabled(bool enabled)
{
    _enabled = enabled;
}

bool RainbowPlayer::isEnabled() const
{
    return _enabled;
}

void RainbowPlayer::setLEDs(Light *leds)
{
    _leds = leds;
}

void RainbowPlayer::setNumLEDs(int numLEDs)
{
    _numLEDs = numLEDs;
    // Revalidate end LED
    if (_endLED >= _numLEDs)
    {
        _endLED = _numLEDs - 1;
    }
}

void RainbowPlayer::setStartLED(int startLED)
{
    _startLED = startLED;
    if (_startLED < 0)
    {
        _startLED = 0;
    }
    // Revalidate end LED
    if (_endLED < _startLED)
    {
        _endLED = _startLED;
    }
}

void RainbowPlayer::setEndLED(int endLED)
{
    _endLED = endLED;
    if (_endLED < _startLED)
    {
        _endLED = _startLED;
    }
    if (_endLED >= _numLEDs)
    {
        _endLED = _numLEDs - 1;
    }
}
