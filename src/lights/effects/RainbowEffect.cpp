#include "RainbowEffect.h"
#include "freertos/LogManager.h"

RainbowEffect::RainbowEffect(int id, float speed, bool reverseDirection, float duration)
    : Effect(id), rainbowPlayer(nullptr, 0, 0, 0, speed, reverseDirection),
    speed(speed), reverseDirection(reverseDirection), duration(duration), elapsed(0.0f)
{

    hasDuration = (duration > 0.0f);
    isInitialized = false;

    LOG_DEBUGF_COMPONENT("Effects", "RainbowEffect: Created with ID %d, speed: %f, reverse: %d, duration: %f", id, speed, reverseDirection, duration);
}

void RainbowEffect::update(float dt)
{
    if (!isActive) return;

    elapsed += dt;

    // Debug logging every 100 updates
    static int debugCounter = 0;
    if (debugCounter++ % 1000 == 0)
    {
        LOG_DEBUGF_COMPONENT("Effects", "RainbowEffect: Update - elapsed: %f, speed: %f, initialized: %d", elapsed, speed, isInitialized);
    }
}

void RainbowEffect::initialize(Light* output, int numLEDs)
{
    this->numLEDs = numLEDs;
    outputBuffer = output;
    
    LOG_DEBUGF_COMPONENT("Effects", "RainbowEffect: Initializing RainbowPlayer with %d LEDs", numLEDs);
    rainbowPlayer.setLEDs(output);
    rainbowPlayer.setNumLEDs(numLEDs);
    rainbowPlayer.setStartLED(0);
    rainbowPlayer.setEndLED(numLEDs - 1);
    rainbowPlayer.setSpeed(speed);
    rainbowPlayer.setDirection(reverseDirection);
    rainbowPlayer.setEnabled(true);
    isInitialized = true;
    LOG_DEBUG_COMPONENT("Effects", "RainbowEffect: RainbowPlayer initialized");
}

void RainbowEffect::render(Light *output)
{
    if (!isActive) return;

    // Debug: Check if RainbowPlayer is enabled
    if (!rainbowPlayer.isEnabled())
    {
        LOG_WARN("RainbowEffect: RainbowPlayer is disabled!");
        return;
    }

    // Update the RainbowPlayer (this is where the actual rainbow logic happens)
    rainbowPlayer.update(0.033f); // Use fixed 30fps for rainbow updates
}

bool RainbowEffect::isFinished() const
{
    if (!isActive) return true;
    if (!hasDuration) return false;
    return elapsed >= duration;
}

void RainbowEffect::setSpeed(float newSpeed)
{
    speed = newSpeed;
    rainbowPlayer.setSpeed(speed);
}

void RainbowEffect::setDirection(bool newReverseDirection)
{
    reverseDirection = newReverseDirection;
    rainbowPlayer.setDirection(reverseDirection);
}

void RainbowEffect::setHue(uint8_t hue)
{
    rainbowPlayer.setHue(hue);
}
