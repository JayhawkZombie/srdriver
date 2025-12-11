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
    void render(Light *output, int numLEDs) override;
    bool isFinished() const override;
    void initialize(Light *output, int numLEDs);
    void spawnPulsePlayer();

private:
    static constexpr size_t MAX_PULSE_PLAYERS = 30;
    std::array<PulsePlayer, MAX_PULSE_PLAYERS> pulsePlayers;
    bool isInitialized = false;
    int nextPulsePlayerIdx = 0;

    RandomIntInRange pulseWidthRange = RandomIntInRange(5, 16);
    RandomFloatInRange pulseSpeedRange = RandomFloatInRange(16.0f, 92.0f);
    RandomFloatInRange pulseTimeBetweenSpawnsRange = RandomFloatInRange(0.5f, 5.0f);
    float pulseTimeSinceLastSpawn = 0.0f;
    RandomBool reverseDirection = RandomBool();

    RandomIntInRange pulseHiColorHueRange = RandomIntInRange(0, 360);
    
    // RandomIntInRange pulseHiColorRange = RandomIntInRange(0, 255);
    // RandomIntInRange pulseLoColorRange = RandomIntInRange(0, 255);
    RandomBool doRepeat = RandomBool();

};
