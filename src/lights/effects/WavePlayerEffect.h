#pragma once

#include "../../Globals.h"
#include "Effect.h"
#include "../WavePlayer.h"
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
    void render(Light *output, int numLEDs) override;
    bool isFinished() const override { return false; }
    void initialize(Light *output, int numLEDs);

private:
    WavePlayer wavePlayer;
    WavePlayerConfig wavePlayerConfig;
    // Light buffer[NUM_LEDS];
    bool isInitialized = false;
    // std::array<LightPanel, 4> lightPanels;
};
