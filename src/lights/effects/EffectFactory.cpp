#include "EffectFactory.h"
#include "WhiteEffect.h"
#include "freertos/LogManager.h"

int EffectFactory::nextEffectId = 1;

std::unique_ptr<Effect> EffectFactory::createEffect(const JsonObject& effectCommand) {
    String effectType = effectCommand["type"];
    JsonObject params = effectCommand["parameters"];
    
    LOG_DEBUG("EffectFactory: Creating effect of type: " + effectType);
    
    if (effectType == "white") {
        return createWhiteEffect(params);
    }
    else if (effectType == "wave") {
        return createWaveEffect(params);
    }
    else if (effectType == "rainbow") {
        return createRainbowEffect(params);
    }
    else {
        LOG_ERROR("EffectFactory: Unknown effect type: " + effectType);
        return nullptr;
    }
}

std::unique_ptr<Effect> EffectFactory::createWhiteEffect(const JsonObject& params) {
    int brightness = params["brightness"] | 255;
    float duration = params["duration"] | -1.0f;
    
    LOG_DEBUG("EffectFactory: Creating white effect - brightness: " + String(brightness) + 
              ", duration: " + String(duration));
    
    return std::unique_ptr<WhiteEffect>(new WhiteEffect(generateEffectId(), brightness, duration));
}

std::unique_ptr<Effect> EffectFactory::createWaveEffect(const JsonObject& params) {
    // TODO: Implement wave effect when we integrate with existing WavePlayer
    LOG_DEBUG("EffectFactory: Wave effect not yet implemented");
    return nullptr;
}

std::unique_ptr<Effect> EffectFactory::createRainbowEffect(const JsonObject& params) {
    // TODO: Implement rainbow effect when we integrate with existing RainbowPlayer
    LOG_DEBUG("EffectFactory: Rainbow effect not yet implemented");
    return nullptr;
}

int EffectFactory::generateEffectId() {
    return nextEffectId++;
}
