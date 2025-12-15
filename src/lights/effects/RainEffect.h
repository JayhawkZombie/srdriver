#pragma once

#include "../../Globals.h"

#include "Effect.h"
#include "../Light.h"
#include "../LightPanel.h"
#include "../RingPlayer.h"
#include "utility/RandomInRange.hpp"
#include <array>
#include <random>
#include <type_traits>


/**
 * Rain effect that wraps the existing LightPanel and RingPlayer
 *
 * Simple wrapper that uses LightPanel and RingPlayer internally
 */
class RainEffect : public Effect {
public:
    RainEffect(int id);
    ~RainEffect() = default;

    // Effect interface
    void update(float dt) override;
    void render(Light *output, int numLEDs) override;
    bool isFinished() const override;
    void setSpawnColumnRange(int minimum, int maximum) { spawnColumnRange = RandomIntInRange(minimum, maximum); }
    void setSpawnRowRange(int minimum, int maximum) { spawnRowRange = RandomIntInRange(minimum, maximum); }
    void setHiLightRange(int minimum, int maximum) { hiLightRange = RandomIntInRange(minimum, maximum); }
    void setLoLightRange(int minimum, int maximum) { loLightRange = RandomIntInRange(minimum, maximum); }
    void setRingWidthRange(float minimum, float maximum) { ringWidthRange = RandomFloatInRange(minimum, maximum); }
    void setLifetimeRange(float minimum, float maximum) { lifetimeRange = RandomFloatInRange(minimum, maximum); }   
    void setAmplitudeRange(float minimum, float maximum) { amplitudeRange = RandomFloatInRange(minimum, maximum); }
    void setOddsOfRadiating(int odds) { oddsOfRadiating = odds; }
    void setSpeedFactor(float factor) { speedFactor = factor; }
    void setSpawnTime(float time) { spawnTime = time; }
    void setTStartFactor(float factor) { tStartFactor = factor; }
    void setTStartMod(int mod) { tStartMod = mod; }

    void initialize(Light *output, int numLEDs);
private:
    // Light buffer[NUM_LEDS];
    bool isInitialized = false;
    // std::array<LightPanel, 4> lightPanels;
    std::array<RingPlayer, 30> ringPlayers;

    RandomIntInRange spawnColumnRange = RandomIntInRange(-8, 38);
    RandomIntInRange spawnRowRange = RandomIntInRange(-8, 38);
    RandomIntInRange hiLightRange = RandomIntInRange(80, 160);
    RandomIntInRange loLightRange = RandomIntInRange(16, 80);
    RandomFloatInRange ringWidthRange = RandomFloatInRange(1.f, 8.f);
    RandomFloatInRange lifetimeRange = RandomFloatInRange(0.5f, 2.0f);
    RandomFloatInRange amplitudeRange = RandomFloatInRange(0.3f, 1.0f);
    // RandomFloatInRange spawnTimeRange = RandomFloatInRange(0.1f, 0.5f);

    float tStart = 0.14f;
    float tElapStart = 0.f;
    int oddsOfRadiating = 3; // 1 in 3 chance to radiate
    int idxStartNext = 0;
    int numRpPlaying = 0;
    float fadeRratio = 1.6f;
    float fadeWratio = 1.6f;
    float spawnTime = 0.5f;// average rate of 2 per second
    float speedFactor = 1.0f;// modulates randomly assigned value
    float tStartFactor = 2.f;
    int tStartMod = 1000;
};

