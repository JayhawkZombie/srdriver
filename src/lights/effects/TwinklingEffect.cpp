#include "TwinklingEffect.h"
#include <cstring>
#include <Arduino.h>
#include <FastLED.h>
#include "freertos/LogManager.h"

TwinklingEffect::TwinklingEffect(int id, int numLEDs, int startLED, int endLED)
    : Effect(id)
    , _numLEDs(numLEDs)
    , _startLED(startLED)
    , _endLED(endLED)
{}

bool TwinklingEffect::isFinished() const
{
    return !_isPlaying;
}

void TwinklingEffect::update(float dt)
{
    // if (!isActive || !_isPlaying)
    //     return;

    static unsigned long updateCount = 0;
    static unsigned long lastLogTime = 0;
    static unsigned long lastUpdateTime = millis();

    updateCount++;
    unsigned long currentTime = millis();

    lastUpdateTime = currentTime;

    // Update existing active stars
    for (int i = 0; i < MAX_STARS; i++)
    {
        if (!_stars[i].isActive)
            continue;

        // Update timer
        _stars[i].timer += dt;

        if (!_stars[i].isFadingOut)
        {
            // Star is in fade-in phase
            _stars[i].brightness += _fadeInSpeed * dt;
            if (_stars[i].brightness > _maxStarBrightness)
                _stars[i].brightness = _maxStarBrightness;

            // Check if it should start fading out
            if (_stars[i].timer >= _stars[i].duration)
            {
                _stars[i].isFadingOut = true;
                _stars[i].timer = 0.0f; // Reset timer for fade out
            }
        }
        else
        {
            // Star is in fade-out phase
            _stars[i].brightness -= _fadeOutSpeed * dt;
            if (_stars[i].brightness <= 0.0f)
            {
                _stars[i].brightness = 0.0f;
                _stars[i].isActive = false; // Mark as inactive
                _activeStarCount--;
            }
        }

        // // Apply the star to the LED with safety checks
        // if (_stars[i].brightness > 0.0f && _stars[i].ledIndex >= 0 && _stars[i].ledIndex < _numLEDs && _leds != nullptr)
        // {
        //     _leds[_stars[i].ledIndex].r = (uint8_t) (_stars[i].color.r * _stars[i].brightness);
        //     _leds[_stars[i].ledIndex].g = (uint8_t) (_stars[i].color.g * _stars[i].brightness);
        //     _leds[_stars[i].ledIndex].b = (uint8_t) (_stars[i].color.b * _stars[i].brightness);
        // }
    }

    // Try to spawn new stars using the selected method
    if (_useTimerSpawn)
    {
        spawnWithTimer(dt);
    }
    else
    {
        spawnWithChance(dt);
    }
}

void TwinklingEffect::render(Light* output, int numLEDs)
{
    // if (!isActive)
    //     return;

    // for (int i = 0; i < numLEDs; i++)
    // {
    //     output[i] = Light(255, 0, 0);
    // }

    for (int i = 0; i < MAX_STARS; i++)
    {
        if (!_stars[i].isActive)
            continue;

        output[_stars[i].ledIndex].r = uint8_t(_stars[i].color.r * _stars[i].brightness);
        output[_stars[i].ledIndex].g = uint8_t(_stars[i].color.g * _stars[i].brightness);
        output[_stars[i].ledIndex].b = uint8_t(_stars[i].color.b * _stars[i].brightness);
    }
}

void TwinklingEffect::init()
{
    // Reset all stars to inactive state
    for (int i = 0; i < MAX_STARS; i++)
    {
        _stars[i].ledIndex = -1;
        _stars[i].timer = 0.0f;
        _stars[i].duration = 0.0f;
        _stars[i].isActive = false;
        _stars[i].color = Light(0, 0, 0);
        _stars[i].brightness = 0.0f;
        _stars[i].isFadingOut = false;
    }
    _activeStarCount = 0;
    _isPlaying = true;
    setActive(true);
}

void TwinklingEffect::spawnWithChance(float dt)
{
    // Try to spawn new stars (only if we have room and chance allows)
    if (_activeStarCount < MAX_STARS && random(0, 1000) / 1000.0f < _starChance)
    {
        LOG_DEBUGF_COMPONENT("TwinklingEffect", "TwinklingEffect: Spawning new star with chance: %.3f", _starChance);
        // Find the first available inactive star slot
        for (int i = 0; i < MAX_STARS; i++)
        {
            if (!_stars[i].isActive)
            {
                // Pick a random LED in our range
                int rangeSize = _numLEDs - _startLED + 1;
                int randomLED = _startLED + random(0, rangeSize);

                // Create new star
                _stars[i].ledIndex = randomLED;
                _stars[i].timer = 0.0f;
                _stars[i].duration = random((int) (_minDuration * 1000), (int) (_maxDuration * 1000)) / 1000.0f;
                _stars[i].isActive = true;
                _stars[i].color = generateStarColor();
                _stars[i].brightness = 0.0f;
                _stars[i].isFadingOut = false;
                _activeStarCount++;

                LOG_DEBUGF_COMPONENT("TwinklingEffect", "TwinklingEffect: Created new star with index: %d, color: %s, brightness: %f, duration: %f", randomLED, _stars[i].color.toString().c_str(), _stars[i].brightness, _stars[i].duration);
                break; // Found and created a star, we're done
            }
        }
    }
}

// Helper function to generate a star color using HSV
Light TwinklingEffect::generateStarColor()
{
    // Generate more dramatic colors
    uint8_t hue = random(0, 40);  // 0-40 gives us red to yellow range
    if (random(0, 2) == 0)
    {
        hue = random(160, 200);  // 160-200 gives us blue to cyan range
    }

    uint8_t saturation = random(100, 255);  // High saturation for dramatic color
    uint8_t value = 255;  // Full brightness

    // Convert HSV to RGB using FastLED
    CHSV hsvColor(hue, saturation, value);
    CRGB rgbColor;
    hsv2rgb_rainbow(hsvColor, rgbColor);

    return Light(rgbColor.r, rgbColor.g, rgbColor.b);
}

void TwinklingEffect::spawnWithTimer(float dt)
{
    static float spawnTimer = 0.0f;
    static float nextSpawnTime = 0.0f;

    // Initialize next spawn time if needed
    if (nextSpawnTime == 0.0f)
    {
        nextSpawnTime = random((int) (_minSpawnTime * 1000), (int) (_maxSpawnTime * 1000)) / 1000.0f;
    }

    spawnTimer += dt; // Use actual delta time

    // Check if it's time to spawn a new star
    if (spawnTimer >= nextSpawnTime && _activeStarCount < MAX_STARS)
    {
        // Find the first available inactive star slot
        for (int i = 0; i < MAX_STARS; i++)
        {
            if (!_stars[i].isActive)
            {
                LOG_DEBUGF_COMPONENT("TwinklingEffect", "TwinklingEffect: TIMER: Spawning new star with chance: %.3f", _starChance);
                // Pick a random LED in our range
                int rangeSize = _endLED - _startLED + 1;
                int randomLED = _startLED + random(0, rangeSize);

                // Create new star
                _stars[i].ledIndex = randomLED;
                _stars[i].timer = 0.0f;
                _stars[i].duration = random((int) (_minDuration * 1000), (int) (_maxDuration * 1000)) / 1000.0f;
                _stars[i].isActive = true;
                _stars[i].color = generateStarColor();
                _stars[i].brightness = 0.0f;
                _stars[i].isFadingOut = false;
                _activeStarCount++;

                // Reset timer and set next spawn time
                spawnTimer = 0.0f;
                nextSpawnTime = random((int) (_minSpawnTime * 1000), (int) (_maxSpawnTime * 1000)) / 1000.0f;

                break; // Found and created a star, we're done
            }
        }
    }
}

void TwinklingEffect::setStarChance(float chance)
{
    _starChance = (chance < 0.0f) ? 0.0f : (chance > 1.0f) ? 1.0f : chance;
}

void TwinklingEffect::setDurationRange(float minDuration, float maxDuration)
{
    _minDuration = (minDuration < 0.0f) ? 0.0f : minDuration;
    _maxDuration = (maxDuration < _minDuration) ? _minDuration : maxDuration;
}

void TwinklingEffect::setSpawnTimeRange(float minSpawnTime, float maxSpawnTime)
{
    _minSpawnTime = (minSpawnTime < 0.0f) ? 0.0f : minSpawnTime;
    _maxSpawnTime = (maxSpawnTime < _minSpawnTime) ? _minSpawnTime : maxSpawnTime;
}

void TwinklingEffect::setStarBrightness(float brightness)
{
    _maxStarBrightness = (brightness < 0.0f) ? 0.0f : (brightness > 1.0f) ? 1.0f : brightness;
}

void TwinklingEffect::setFadeSpeeds(float fadeInSpeed, float fadeOutSpeed)
{
    _fadeInSpeed = (fadeInSpeed < 0.0f) ? 0.0f : fadeInSpeed;
    _fadeOutSpeed = (fadeOutSpeed < 0.0f) ? 0.0f : fadeOutSpeed;
}
