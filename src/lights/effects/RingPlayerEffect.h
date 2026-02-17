#pragma once

#include "../../Globals.h"
#include "Effect.h"
#include "../RingPlayer.h"
#include "../LightPanel.h"

/**
 * Ring Player Effect
 *
 * Renders a ring player effect
 */
class RingPlayerEffect : public Effect {
public:
    RingPlayerEffect(int id, RPdata ringPlayerConfig);
    virtual ~RingPlayerEffect() = default;

    // Effect interface
    void update(float dt) override;
    void initialize(Light *output, int numLEDs) override;
    void render(Light *output) override;
    bool isFinished() const override { return false; }

private:
    RingPlayer ringPlayer;
    RPdata ringPlayerConfig;
    int numLEDs;
    Light *outputBuffer;
    bool isInitialized = false;
};
