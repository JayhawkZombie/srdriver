#include "PulsePlayerEffect.h"
#include "freertos/LogManager.h"
#include <FastLED.h>

PulsePlayerEffect::PulsePlayerEffect(int id) : Effect(id) {}


void PulsePlayerEffect::update(float dt) {
    if (!isActive) return;
    if (!isInitialized) return;
    for (auto &pulsePlayer : pulsePlayers) {
        pulsePlayer.update(dt);
    }
    pulseTimeSinceLastSpawn += dt;
    if (pulseTimeSinceLastSpawn >= pulseTimeBetweenSpawnsRange.random()) {
        spawnPulsePlayer();
        pulseTimeSinceLastSpawn = 0.0f;
    }
}

void PulsePlayerEffect::render(Light *output, int numLEDs) {
    if (!isActive) return;
    if (!isInitialized) {
        initialize(output, numLEDs);
        update(0.f);
    }

    // output[0] = Light(255, 0, 0);
}

bool PulsePlayerEffect::isFinished() const {
    return false;
}

void PulsePlayerEffect::initialize(Light *output, int numLEDs) {
    // Init members
    // pulsePlayer.init(output[0], 16, 16, Light(255, 255, 255), Light(0, 0, 0), 8, 32.0f, 1.0f, true);
    for (auto &pulsePlayer : pulsePlayers) {
        const auto pulseWidth = pulseWidthRange.random();
        auto pulseSpeed = pulseSpeedRange.random();
        const auto pulseHiColorHue = pulseHiColorHueRange.random();
        const auto doReverse = reverseDirection.random();

        int startIndex = 0;
        if (pulseSpeed < 0.f) {
            startIndex = 100;
        }
        if (doReverse) {
            pulseSpeed = -pulseSpeed;
        }

        LOG_DEBUGF_COMPONENT("PulsePlayerEffect", "Pulse width: %d, Pulse speed: %.4f", pulseWidth, pulseSpeed);

        const auto pulseHiColor = CHSV(pulseHiColorHue, 255, 255);
        Light pulseHiColorRGB;
        hsv2rgb_raw(pulseHiColor, pulseHiColorRGB);

        pulsePlayer.init(
            output[startIndex],
            numLEDs,
            pulseHiColorRGB,
            pulseWidth,
            pulseSpeed,
            false
        );
    }
    isInitialized = true;
}

void PulsePlayerEffect::spawnPulsePlayer() {
    pulsePlayers[nextPulsePlayerIdx].Start();
    nextPulsePlayerIdx = (nextPulsePlayerIdx + 1) % pulsePlayers.size();
}