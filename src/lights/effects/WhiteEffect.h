#pragma once

#include "Effect.h"

/**
 * Simple white LED effect
 * 
 * Renders all LEDs as white with optional brightness control
 */
class WhiteEffect : public Effect {
public:
    WhiteEffect(int id, int brightness = 255, float duration = -1.0f);
    virtual ~WhiteEffect() = default;
    
    // Effect interface
    void update(float dt) override;
    void initialize(Light* output, int numLEDs) override;
    void render(Light* output) override;
    bool isFinished() const override;
    
    // White effect specific
    void setBrightness(int brightness);
    int getBrightness() const { return currentBrightness; }
    
private:
    int numLEDs;
    int currentBrightness;
    float duration;
    float elapsed;
    bool hasDuration;
};
