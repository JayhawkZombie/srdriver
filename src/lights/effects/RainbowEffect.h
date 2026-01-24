#pragma once

#include "Effect.h"
#include "../players/RainbowPlayer.h"

/**
 * Rainbow effect that wraps the existing RainbowPlayer
 * 
 * Simple wrapper that uses RainbowPlayer internally
 */
class RainbowEffect : public Effect {
public:
    RainbowEffect(int id, float speed = 1.0f, bool reverseDirection = false, float duration = -1.0f);
    ~RainbowEffect() = default;
    
    // Effect interface
    void update(float dt) override;
    void initialize(Light* output, int numLEDs) override;
    void render(Light* output) override;
    bool isFinished() const override;
    
    // Rainbow-specific controls
    void setSpeed(float speed);
    void setDirection(bool reverseDirection);
    void setHue(uint8_t hue);
    
private:
    RainbowPlayer rainbowPlayer;
    int numLEDs;
    Light* outputBuffer;
    float speed;
    bool reverseDirection;
    float duration;
    float elapsed;
    bool hasDuration;
    bool isInitialized;
};
