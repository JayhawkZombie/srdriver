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
    if (millis() >= nextPulsePlayerSpawnTime) {
        spawnPulsePlayer();
        nextPulsePlayerSpawnTime = millis() + pulseTimeBetweenSpawnsRange.random() * 1000.0f;
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
    outputArr = output;
    _numLEDs = numLEDs;
    for (auto &pulsePlayer : pulsePlayers) {
        const auto pulseWidth = pulseWidthRange.random();
        auto pulseSpeed = pulseSpeedRange.random();
        const auto pulseHiColorHue = pulseHiColorHueRange.random();
        const auto doReverse = reverseDirection.random();

        int startIndex = 0;
        // if (doReverse) {
        //     pulseSpeed = -pulseSpeed;
        // }
        // if (pulseSpeed < 0.f) {
        //     startIndex = 400 - 100;
        // }

        LOG_DEBUGF_COMPONENT("PulsePlayerEffect", "Pulse width: %d, Pulse speed: %.4f", pulseWidth, pulseSpeed);

        const auto pulseHiColor = CHSV(pulseHiColorHue, 255, 255);
        Light pulseHiColorRGB;
        hsv2rgb_raw(pulseHiColor, pulseHiColorRGB);

        pulsePlayer.init(
            outputArr[startIndex],
            numLEDs,
            // 300,
            pulseHiColorRGB,
            pulseWidth,
            pulseSpeed,
            false
        );
    }
    nextPulsePlayerSpawnTime = millis() + pulseTimeBetweenSpawnsRange.random() * 1000.0f;
    isInitialized = true;
}

void PulsePlayerEffect::spawnPulsePlayer() {
    // Safety checks
    if (!outputArr || _numLEDs <= 0) {
        LOG_ERROR_COMPONENT("PulsePlayerEffect", "Cannot spawn pulse player - outputArr is null or _numLEDs is invalid");
        return;
    }
    
    LOG_DEBUGF_COMPONENT("PulsePlayerEffect", "Spawning pulse player %d", nextPulsePlayerIdx);
    PulsePlayer *player = &pulsePlayers[nextPulsePlayerIdx];
    const auto pulseHiColor = CHSV(pulseHiColorHueRange.random(), 255, 255);
    Light pulseHiColorRGB;
    hsv2rgb_raw(pulseHiColor, pulseHiColorRGB);
    const auto doReverse = reverseDirection.random();
    int pulseWidth = pulseWidthRange.random();
    float pulseSpeed = pulseSpeedRange.random();
    
    // Ensure pulseWidth is valid (at least 1)
    if (pulseWidth < 1) {
        pulseWidth = 1;
    }
    
    int startIndex = 0;
    if (doReverse) {
        pulseSpeed = -pulseSpeed;
    }
    if (pulseSpeed < 0.f) {
        // Calculate startIndex safely - ensure it's within bounds
        startIndex = _numLEDs > 100 ? (_numLEDs - 100) : 0;
    }
    
    LOG_DEBUGF_COMPONENT("PulsePlayerEffect", "Pulse player params: pulseHiColor: %d, reverse: %d, pulseWidth: %d, pulseSpeed: %f, startIndex: %d", pulseHiColor.h, doReverse, pulseWidth, pulseSpeed, startIndex);
    
    // Use startIndex (was calculated but never used - bug fix)
    if (startIndex < 0 || startIndex >= _numLEDs) {
        startIndex = 0;  // Fallback to safe value
    }
    
    player->init(
        outputArr[0],
        _numLEDs,
        pulseHiColorRGB,
        pulseWidth,
        pulseSpeed,
        false
    );
    player->Start();
    nextPulsePlayerIdx = (nextPulsePlayerIdx + 1) % pulsePlayers.size();
}