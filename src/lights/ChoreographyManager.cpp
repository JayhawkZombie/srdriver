#include "ChoreographyManager.h"
#include "effects/EffectManager.h"
#include "effects/EffectFactory.h"
#include "../PatternManager.h"
#include "../UserPreferences.h"
#include "../Globals.h"
#include "LEDManager.h"

ChoreographyManager::ChoreographyManager() 
    : active(false), effectManager(nullptr), choreographyStartTime(0), choreographyDuration(0) {
    LOG_DEBUG_COMPONENT("ChoreographyManager", "Initializing");
}

ChoreographyManager::~ChoreographyManager() {
    if (active) {
        stop();
    }
    LOG_DEBUG_COMPONENT("ChoreographyManager", "Destroying");
}

void ChoreographyManager::startChoreography(const JsonObject& command, EffectManager* em) {
    if (!em) {
        LOG_ERROR_COMPONENT("ChoreographyManager", "EffectManager is null");
        return;
    }
    
    effectManager = em;
    active = true;
    choreographyStartTime = millis();
    
    // Save current state
    saveCurrentState();
    
    // Parse background effect (optional)
    if (command.containsKey("bg_effect")) {
        JsonObject bgEffect = command["bg_effect"].as<JsonObject>();
        auto effect = EffectFactory::createEffect(bgEffect);
        if (effect) {
            effectManager->removeAllEffects();
            extern Light BlendLightArr[];
            effectManager->addEffect(std::move(effect), BlendLightArr, NUM_LEDS);
        }
    }
    
    // Parse beat patterns
    beatPatterns.clear();
    if (command.containsKey("beats") && command["beats"].is<JsonArray>()) {
        JsonArray beats = command["beats"].as<JsonArray>();
        for (JsonObject beat : beats) {
            BeatPattern pattern;
            pattern.id = beat.containsKey("id") ? beat["id"].as<String>() : "";
            pattern.bps = beat.containsKey("bps") ? beat["bps"].as<float>() : 1.0f;
            
            // Parse brightness params
            JsonObject params = beat.containsKey("params") ? beat["params"].as<JsonObject>() : JsonObject();
            pattern.baseBrightness = params.containsKey("base") ? params["base"].as<int>() : 128;
            pattern.peakBrightness = params.containsKey("peak") ? params["peak"].as<int>() : 255;
            pattern.pulseDuration = params.containsKey("pulse_duration") ? params["pulse_duration"].as<unsigned long>() : 250;
            
            pattern.startTime = beat.containsKey("start_t") ? beat["start_t"].as<unsigned long>() : 0;
            pattern.duration = beat.containsKey("duration") ? beat["duration"].as<unsigned long>() : 0;
            pattern.lastBeatTime = 0;
            pattern.active = false;
            
            beatPatterns.push_back(pattern);
        }
    }
    
    // Get choreography duration
    choreographyDuration = command.containsKey("duration") ? 
        command["duration"].as<unsigned long>() : 0;
    
    LOG_DEBUGF_COMPONENT("ChoreographyManager", 
        "Started choreography with %d beats, duration %lu ms",
        beatPatterns.size(), choreographyDuration);
}

void ChoreographyManager::update(float dt) {
    if (!active) return;
    
    unsigned long elapsed = millis() - choreographyStartTime;
    
    // Check if choreography is complete
    if (choreographyDuration > 0 && elapsed >= choreographyDuration) {
        // Check if all beat patterns are finished
        bool allFinished = true;
        for (const auto& beat : beatPatterns) {
            if (beat.active) {
                allFinished = false;
                break;
            }
        }
        
        if (allFinished) {
            stop();
            return;
        }
    }
    
    // Update beat patterns
    updateBeatPatterns();
}

void ChoreographyManager::render(Light* outputBuffer, int numLEDs, int gridRows, int gridCols) {
    // Nothing to render for brightness pulsing - it's handled by BrightnessController
    (void)outputBuffer;
    (void)numLEDs;
    (void)gridRows;
    (void)gridCols;
}

void ChoreographyManager::stop() {
    if (!active) return;
    
    active = false;
    
    // Stop all beat patterns
    for (auto& beat : beatPatterns) {
        beat.active = false;
    }
    
    // Stop any active brightness pulses
    BrightnessController* bc = BrightnessController::getInstance();
    if (bc) {
        bc->stopPulse();
    }
    
    // Restore previous state
    restorePreviousState();
    
    LOG_DEBUG_COMPONENT("ChoreographyManager", "Choreography stopped, state restored");
}

void ChoreographyManager::updateBeatPatterns() {
    unsigned long now = millis();
    unsigned long elapsed = now - choreographyStartTime;
    
    BrightnessController* bc = BrightnessController::getInstance();
    
    for (auto& beat : beatPatterns) {
        // Check if beat pattern should start
        if (!beat.active && elapsed >= beat.startTime) {
            beat.active = true;
            beat.lastBeatTime = now;
        }
        
        // Update active beat patterns
        if (beat.active) {
            // Check if duration expired
            unsigned long beatElapsed = elapsed - beat.startTime;
            if (beatElapsed >= beat.duration) {
                beat.active = false;
                // Set brightness to base when pattern ends
                // Don't call stopPulse() here - let any new active patterns handle their own pulses
                if (bc) {
                    bc->setBrightness(beat.baseBrightness);
                }
                continue;
            }
            
            // Check if it's time for next beat
            // BrightnessController handles the full pulse cycle, we just need to check if it's done
            unsigned long interval = beat.getIntervalMs();
            unsigned long timeSinceLastBeat = now - beat.lastBeatTime;
            
            // Trigger next pulse if interval has passed - timeline always takes precedence
            if (timeSinceLastBeat >= interval) {
                executeBrightnessPulse(beat);
                beat.lastBeatTime = now;
            }
        }
    }
}

void ChoreographyManager::executeBrightnessPulse(BeatPattern& beat) {
    BrightnessController* bc = BrightnessController::getInstance();
    if (!bc) {
        LOG_ERROR_COMPONENT("ChoreographyManager", "BrightnessController not available");
        return;
    }
    
    // Let BrightnessController handle the full pulse cycle (base -> peak -> base)
    bc->startPulseCycle(beat.baseBrightness, beat.peakBrightness, beat.pulseDuration);
    
    LOG_DEBUGF_COMPONENT("ChoreographyManager", 
        "Executing brightness pulse cycle: base=%d, peak=%d, duration=%lu ms",
        beat.baseBrightness, beat.peakBrightness, beat.pulseDuration);
}

void ChoreographyManager::saveCurrentState() {
    extern DeviceState deviceState;
    savedState.effectType = deviceState.currentEffectType;
    savedState.effectParams = deviceState.currentEffectParams;
    savedState.brightness = deviceState.brightness;
    savedState.valid = true;
    
    LOG_DEBUGF_COMPONENT("ChoreographyManager", 
        "Saved state: effect=%s, brightness=%d",
        savedState.effectType.c_str(), savedState.brightness);
}

void ChoreographyManager::restorePreviousState() {
    if (!savedState.valid) return;
    
    extern DeviceState deviceState;
    
    // Restore brightness
    BrightnessController* bc = BrightnessController::getInstance();
    if (bc) {
        bc->setBrightness(savedState.brightness);
    }
    deviceState.brightness = savedState.brightness;
    
    // Restore effect
    if (savedState.effectType.length() > 0 && effectManager) {
        String effectCommand = "{\"t\":\"effect\",\"e\":{\"t\":\"" + savedState.effectType + "\"";
        if (savedState.effectParams.length() > 0) {
            effectCommand += ",\"p\":" + savedState.effectParams;
        }
        effectCommand += "}}";
        
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, effectCommand);
        if (!error) {
            JsonObject command = doc.as<JsonObject>();
            // Use LEDManager to handle the command
            extern LEDManager* g_ledManager;
            if (g_ledManager) {
                g_ledManager->handleCommand(command);
            } else {
                LOG_ERROR_COMPONENT("ChoreographyManager", "g_ledManager not available for state restoration");
            }
        }
    }
    
    savedState.valid = false;
    LOG_DEBUG_COMPONENT("ChoreographyManager", "State restored");
}
