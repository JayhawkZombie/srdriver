#include "Globals.h"
#include "PatternLayer.h"
#include "../PulsePlayer.h"

PatternLayer::PatternLayer(PulsePlayer* pp, Light* buffer)
    : pulsePlayer(pp), blendBuffer(buffer) {
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
    
    // Apply pulse as mask to existing output
    for (int i = 0; i < numLeds; i++) {
        float pulseIntensity = blendBuffer[i].r / 255.0f;
        output[i].r = (uint8_t)(output[i].r * pulseIntensity);
        output[i].g = (uint8_t)(output[i].g * pulseIntensity);
        output[i].b = (uint8_t)(output[i].b * pulseIntensity);
    }
}
