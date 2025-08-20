#include "MainLayer.h"
#include "../WavePlayer.h"
#include "../RainbowPlayer.h"

MainLayer::MainLayer(WavePlayer* wp, RainbowPlayer* rp1, RainbowPlayer* rp2)
    : wavePlayer(wp), rainbowPlayer1(rp1), rainbowPlayer2(rp2) {
}

void MainLayer::update(float dt) {
    if (wavePlayer) wavePlayer->update(dt);
    if (rainbowPlayer1) rainbowPlayer1->update(dt);
    if (rainbowPlayer2) rainbowPlayer2->update(dt);
}

void MainLayer::render(Light* output, int numLeds) {
    // For now, just copy the main pattern buffer
    // This will be replaced with actual rendering logic
    for (int i = 0; i < numLeds; i++) {
        output[i] = Light(0, 0, 0); // Placeholder
    }
} 