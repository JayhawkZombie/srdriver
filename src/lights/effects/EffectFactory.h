#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <memory>
#include "Effect.h"

/**
 * Factory for creating effects from JSON commands
 * 
 * Handles parsing JSON commands and creating appropriate effect instances
 */
class EffectFactory {
public:
    // Create effect from JSON command
    static std::unique_ptr<Effect> createEffect(const JsonObject& effectCommand);
    
    // Create specific effect types
    static std::unique_ptr<Effect> createWhiteEffect(const JsonObject& params);
    static std::unique_ptr<Effect> createSolidColorEffect(const JsonObject& params);
    static std::unique_ptr<Effect> createWaveEffect(const JsonObject& params);
    static std::unique_ptr<Effect> createRainbowEffect(const JsonObject& params);
    
private:
    static int nextEffectId;
    static int generateEffectId();
};
