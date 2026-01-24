#pragma once

#include "../../Globals.h"
#include "Effect.h"
#include "../PulsePlayer.h"
#include "../LightPanel.h"
#include "utility/RandomInRange.hpp"
#include "utility/RandomBool.hpp"

/**
 * Pulse Player Effect
 *
 * Renders a pulse player effect
 */

class PulsePlayerEffect : public Effect {
public:
    PulsePlayerEffect(int id);
    virtual ~PulsePlayerEffect() = default;

    // Effect interface
    void update(float dt) override;
    void initialize(Light* output, int numLEDs) override;
    void initializePulsePlayers();
    void render(Light *output) override;
    bool isFinished() const override;
    void spawnPulsePlayer();

    void setPulseWidthRange(int minimum, int maximum) { pulseWidthRange = RandomIntInRange(minimum, maximum); }
    void setPulseSpeedRange(float minimum, float maximum) { pulseSpeedRange = RandomFloatInRange(minimum, maximum); }
    void setPulseTimeBetweenSpawnsRange(float minimum, float maximum) { pulseTimeBetweenSpawnsRange = RandomFloatInRange(minimum, maximum); }
    void setPulseHiColorHueRange(int minimum, int maximum) { pulseHiColorHueRange = RandomIntInRange(minimum, maximum); }

private:
    static constexpr size_t MAX_PULSE_PLAYERS = 40;
    Light *outputArr = nullptr;
    int _numLEDs = 0;
    std::array<PulsePlayer, MAX_PULSE_PLAYERS> pulsePlayers;
    bool isInitialized = false;
    int nextPulsePlayerIdx = 0;
    float nextPulsePlayerSpawnTime = 0.0f;

    RandomIntInRange pulseWidthRange = RandomIntInRange(5, 16);
    RandomFloatInRange pulseSpeedRange = RandomFloatInRange(16.0f, 92.0f);
    RandomFloatInRange pulseTimeBetweenSpawnsRange = RandomFloatInRange(0.5f, 6.0f);
    RandomBool reverseDirection = RandomBool();

    RandomIntInRange pulseHiColorHueRange = RandomIntInRange(0, 360);
    
    // RandomIntInRange pulseHiColorRange = RandomIntInRange(0, 255);
    // RandomIntInRange pulseLoColorRange = RandomIntInRange(0, 255);
    RandomBool doRepeat = RandomBool();

};
