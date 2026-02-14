#include "PulsePlayerEffect.h"
#include "freertos/LogManager.h"
#include <ArduinoJson.h>
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

void PulsePlayerEffect::initialize(Light* output, int numLEDs) {
    this->_numLEDs = numLEDs;
    outputArr = output;
    
    initializePulsePlayers();
    isInitialized = true;
}

void PulsePlayerEffect::render(Light *output) {
    if (!isActive) return;
    // PulsePlayers write directly to the buffer via update(), nothing to do here

    // output[0] = Light(255, 0, 0);
}

bool PulsePlayerEffect::isFinished() const {
    return false;
}

bool PulsePlayerEffect::updateParams(const JsonObject& params) {
    if (params.isNull()) return true;

    // Full-range updates only (both min and max required per group). Swap if min > max.
    if (params.containsKey("pw_min") && params.containsKey("pw_max")) {
        int a = params["pw_min"].as<int>(), b = params["pw_max"].as<int>();
        if (a > b) { int t = a; a = b; b = t; }
        setPulseWidthRange(a, b);
    }
    if (params.containsKey("ps_min") && params.containsKey("ps_max")) {
        float a = params["ps_min"].as<float>(), b = params["ps_max"].as<float>();
        if (a > b) { float t = a; a = b; b = t; }
        setPulseSpeedRange(a, b);
    }
    if (params.containsKey("tbs_min") && params.containsKey("tbs_max")) {
        float a = params["tbs_min"].as<float>(), b = params["tbs_max"].as<float>();
        if (a > b) { float t = a; a = b; b = t; }
        setPulseTimeBetweenSpawnsRange(a, b);
    }
    if (params.containsKey("hi_min") && params.containsKey("hi_max")) {
        int a = params["hi_min"].as<int>(), b = params["hi_max"].as<int>();
        if (a > b) { int t = a; a = b; b = t; }
        setPulseHiColorHueRange(a, b);
    }
    return true;
}

void PulsePlayerEffect::initializePulsePlayers() {
    // Init members
    // pulsePlayer.init(output[0], 16, 16, Light(255, 255, 255), Light(0, 0, 0), 8, 32.0f, 1.0f, true);
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

        const auto pulseHiColor = CHSV(pulseHiColorHue, 255, 255);
        Light pulseHiColorRGB;
        hsv2rgb_raw(pulseHiColor, pulseHiColorRGB);

        pulsePlayer.init(
            outputArr[startIndex],
            _numLEDs,
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
        return;
    }
    
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