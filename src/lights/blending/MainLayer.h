#pragma once

#include "Layer.h"

// Forward declarations
class WavePlayer;
class RainbowPlayer;

class MainLayer : public Layer {
private:
    WavePlayer* wavePlayer;
    RainbowPlayer* rainbowPlayer1;
    RainbowPlayer* rainbowPlayer2;

public:
    MainLayer(WavePlayer* wp, RainbowPlayer* rp1, RainbowPlayer* rp2);
    void update(float dt) override;
    void render(Light* output, int numLeds) override;
}; 