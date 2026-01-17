#include "WhiteEffect.h"
#include "freertos/LogManager.h"

WhiteEffect::WhiteEffect(int id, int brightness, float duration) 
    : Effect(id), currentBrightness(brightness), duration(duration), elapsed(0.0f) {
    
    hasDuration = (duration > 0.0f);
    
    LOG_DEBUG("WhiteEffect: Created with ID " + String(id) + 
              ", brightness " + String(brightness) + 
              ", duration " + String(duration));
}

void WhiteEffect::update(float dt) {
    if (!isActive) return;
    
    elapsed += dt;
    
    // Debug logging every 100 updates
    static int debugCounter = 0;
    if (debugCounter++ % 100 == 0) {
        LOG_DEBUG("WhiteEffect: Update - elapsed: " + String(elapsed) + 
                  ", brightness: " + String(currentBrightness));
    }
}

void WhiteEffect::initialize(Light* output, int numLEDs) {
    this->numLEDs = numLEDs;
    (void)output; // Not needed for simple effects
    LOG_DEBUGF_COMPONENT("WhiteEffect", "Initialized with %d LEDs", numLEDs);
}

void WhiteEffect::render(Light* output) {
    if (!isActive) return;
    
    // Render pure white LEDs (brightness is controlled by FastLED.setBrightness globally)
    for (int i = 0; i < numLEDs; i++) {
        output[i] = Light(255, 255, 255);
    }
}

bool WhiteEffect::isFinished() const {
    if (!isActive) return true;
    if (!hasDuration) return false;
    return elapsed >= duration;
}

void WhiteEffect::setBrightness(int brightness) {
    currentBrightness = constrain(brightness, 0, 255);
    LOG_DEBUG("WhiteEffect: Brightness set to " + String(currentBrightness));
}
