#pragma once

#include "Layer.h"

// Forward declarations
class PulsePlayer;

class PatternLayer : public Layer {
private:
    PulsePlayer* pulsePlayer;
    Light* blendBuffer;

public:
    PatternLayer(PulsePlayer* pp, Light* buffer);
    void update(float dt) override;
    void render(Light* output, int numLeds) override;
};
