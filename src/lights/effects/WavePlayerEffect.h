#pragma once

#include "../../Globals.h"
#include "Effect.h"
#include "../players/WavePlayer.h"
#include "../LightPanel.h"

/**
 * Wave Player Effect
 *
 * Renders a wave player effect
 */
class WavePlayerEffect : public Effect {
public:
    WavePlayerEffect(int id, WavePlayerConfig wavePlayerConfig);
    virtual ~WavePlayerEffect() = default;

    // Effect interface
    void update(float dt) override;
    void initialize(Light* output, int numLEDs) override;
    void render(Light *output) override;
    bool isFinished() const override { return false; }

private:
    WavePlayer wavePlayer;
    WavePlayerConfig wavePlayerConfig;
    int numLEDs;
    Light* outputBuffer;
    // Light buffer[NUM_LEDS];
    bool isInitialized = false;
    // std::array<LightPanel, 4> lightPanels;
};
