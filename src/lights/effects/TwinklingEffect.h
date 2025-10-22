#pragma once

#include "../Light.h"
#include "Effect.h"

class TwinklingEffect : public Effect
{
public:
    TwinklingEffect(int id, int numLEDs, int startLED, int endLED);
    ~TwinklingEffect() = default;

    void update(float dt) override;
    void render(Light *output, int numLEDs) override;
    bool isFinished() const override;

private:
    static const int MAX_STARS = 20;  // Maximum number of active stars at once

    // Fixed-size arrays for star data
    struct Star {
        int ledIndex;           // Which LED this star is on
        float timer;            // Current timer
        float duration;         // How long this star stays visible
        bool isActive;          // Whether this star is currently active
        Light color;            // Color of this star
        float brightness;       // Current brightness (0.0 to 1.0)
        bool isFadingOut;       // Whether star is in fade-out phase
    };

    Star _stars[MAX_STARS];     // Fixed array of stars
    int _activeStarCount;       // Number of currently active stars
    int _numLEDs;
    int _startLED;
    int _endLED;
    bool _isPlaying = false;

    float _minDuration;         // Minimum time between star appearances
    float _maxDuration;         // Maximum time between star appearances
    float _minSpawnTime;        // Minimum time between spawns (for timer-based)
    float _maxSpawnTime;        // Maximum time between spawns (for timer-based)
    float _starChance;          // Probability of new star appearing (0.0 to 1.0)
    float _maxStarBrightness;   // Maximum brightness of stars (0.0 to 1.0)
    float _fadeInSpeed;         // How fast stars fade in (brightness per second)
    float _fadeOutSpeed;        // How fast stars fade out (brightness per second)

    bool _enabled;

public:
    // LEDTwinkler(Light *leds, int numLEDs, int startLED, int endLED);
    // ~LEDTwinkler();

    // void update(float dt);
    void init();
    // void setEnabled(bool enabled);
    void setStarChance(float chance);  // 0.0 = no stars, 1.0 = many stars
    void setDurationRange(float minDuration, float maxDuration);
    void setSpawnTimeRange(float minSpawnTime, float maxSpawnTime);  // For timer-based spawning
    void setStarBrightness(float brightness);  // 0.0 to 1.0
    void setFadeSpeeds(float fadeInSpeed, float fadeOutSpeed);  // Brightness per second
    // void setLEDs(Light *leds);
    void setSpawnMethod(bool useTimer);  // true = spawnWithTimer, false = spawnWithChance

    bool isEnabled() const { return _enabled; }

private:
    Light generateStarColor();  // Helper function to generate HSV-based star colors
    void spawnWithChance(float dt);     // Spawn stars based on random chance
    void spawnWithTimer(float dt);      // Spawn stars based on timer intervals
    bool _useTimerSpawn;        // Whether to use timer-based spawning
};
