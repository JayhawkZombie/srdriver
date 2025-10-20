#include "RainbowEffect.h"
#include "freertos/LogManager.h"

RainbowEffect::RainbowEffect(int id, float speed, bool reverseDirection, float duration) 
    : Effect(id), rainbowPlayer(nullptr, 0, 0, 0, speed, reverseDirection), 
      speed(speed), reverseDirection(reverseDirection), duration(duration), elapsed(0.0f) {
    
    hasDuration = (duration > 0.0f);
    isInitialized = false;
    
    LOG_DEBUG("RainbowEffect: Created with ID " + String(id) + 
              ", speed: " + String(speed) + 
              ", reverse: " + String(reverseDirection) + 
              ", duration: " + String(duration));
}

void RainbowEffect::update(float dt) {
    if (!isActive) return;
    
    elapsed += dt;
    
    // Initialize RainbowPlayer if not done yet (we need the LED buffer for this)
    // But we can't initialize here because we don't have the output buffer yet
    // So we'll do it in render() instead
    
    // Debug logging every 100 updates
    static int debugCounter = 0;
    if (debugCounter++ % 100 == 0) {
        LOG_DEBUG("RainbowEffect: Update - elapsed: " + String(elapsed) + 
                  ", speed: " + String(speed) + ", initialized: " + String(isInitialized));
    }
}

void RainbowEffect::render(Light* output, int numLEDs) {
    if (!isActive) return;
    
    // Initialize RainbowPlayer with the output buffer if not done yet
    if (!isInitialized) {
        LOG_DEBUG("RainbowEffect: Initializing RainbowPlayer with " + String(numLEDs) + " LEDs");
        rainbowPlayer.setLEDs(output);
        rainbowPlayer.setNumLEDs(numLEDs);
        rainbowPlayer.setStartLED(0);
        rainbowPlayer.setEndLED(numLEDs - 1);
        rainbowPlayer.setSpeed(speed);
        rainbowPlayer.setDirection(reverseDirection);
        rainbowPlayer.setEnabled(true);
        isInitialized = true;
        LOG_DEBUG("RainbowEffect: RainbowPlayer initialized");
    }
    
    // Debug: Check if RainbowPlayer is enabled
    if (!rainbowPlayer.isEnabled()) {
        LOG_WARN("RainbowEffect: RainbowPlayer is disabled!");
        return;
    }
    
    // Update the RainbowPlayer (this is where the actual rainbow logic happens)
    rainbowPlayer.update(0.033f); // Use fixed 30fps for rainbow updates
    
    // Debug: Log first few LED values
    static int debugCounter = 0;
    if (debugCounter++ % 50 == 0) {
        LOG_DEBUG("RainbowEffect: Render - LED 0: r=" + String(output[0].r) + 
                  " g=" + String(output[0].g) + " b=" + String(output[0].b));
    }
}

bool RainbowEffect::isFinished() const {
    if (!isActive) return true;
    if (!hasDuration) return false;
    return elapsed >= duration;
}

void RainbowEffect::setSpeed(float newSpeed) {
    speed = newSpeed;
    rainbowPlayer.setSpeed(speed);
}

void RainbowEffect::setDirection(bool newReverseDirection) {
    reverseDirection = newReverseDirection;
    rainbowPlayer.setDirection(reverseDirection);
}

void RainbowEffect::setHue(uint8_t hue) {
    rainbowPlayer.setHue(hue);
}
