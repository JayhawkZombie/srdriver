#include "Globals.h"
#include "PatternLayer.h"
#include "../PulsePlayer.h"

PatternLayer::PatternLayer(PulsePlayer* pp, Light* buffer)
    : pulsePlayer(pp), blendBuffer(buffer) {
    setBlender(&selectiveMaskBlender);
}

void PatternLayer::update(float dt) {
    if (pulsePlayer) {
        // Clear blend buffer
        for (int i = 0; i < NUM_LEDS; i++) {
            blendBuffer[i] = Light(0, 0, 0);
        }
        pulsePlayer->update(dt);
    }
}

void PatternLayer::render(Light* output, int numLeds) {
    if (!pulsePlayer || !blendBuffer) return;
    
    // Copy pulse output to temp buffer for blending
    for (int i = 0; i < numLeds; i++) {
        output[i] = blendBuffer[i];
    }
}
