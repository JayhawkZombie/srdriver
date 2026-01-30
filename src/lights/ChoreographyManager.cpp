#include "ChoreographyManager.h"
#include "effects/EffectManager.h"
#include "effects/EffectFactory.h"
#include "../PatternManager.h"
#include "../UserPreferences.h"
#include "../Globals.h"
#include "LEDManager.h"
#include "RingPlayer.h"

// Forward declaration for parseColorString (defined in EffectFactory.cpp)
void parseColorString(const String& colorString, Light& color);

ChoreographyManager::ChoreographyManager() 
    : active(false), effectManager(nullptr), choreographyStartTime(0), choreographyDuration(0),
      outputBuffer(nullptr), gridRows(32), gridCols(32), ringPlayersInitialized(false) {
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
    
    // Reset ring players for new choreography
    if (ringPlayersInitialized) {
        for (auto& rp : ringPlayerPool) {
            rp.isPlaying = false;
        }
    }
    
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
            
            // Parse action type
            pattern.action = beat.containsKey("action") ? beat["action"].as<String>() : "brightness_pulse";
            
            // Store params as JSON string for later parsing
            if (beat.containsKey("params") && beat["params"].is<JsonObject>()) {
                JsonObject params = beat["params"].as<JsonObject>();
                // Serialize params to JSON string
                String paramsStr;
                serializeJson(params, paramsStr);
                pattern.paramsJson = paramsStr;
            } else {
                pattern.paramsJson = "{}";
            }
            
            pattern.startTime = beat.containsKey("start_t") ? beat["start_t"].as<unsigned long>() : 0;
            
            // Support both "duration" and "end_t" - convert duration to endTime if provided
            if (beat.containsKey("duration")) {
                unsigned long duration = beat["duration"].as<unsigned long>();
                pattern.endTime = pattern.startTime + duration;
            } else if (beat.containsKey("end_t")) {
                pattern.endTime = beat["end_t"].as<unsigned long>();
            } else {
                // Default to 0 if neither provided (pattern never ends)
                pattern.endTime = 0;
            }
            
            pattern.lastBeatTime = 0;
            pattern.active = false;
            
            beatPatterns.push_back(pattern);
        }
    }
    
    // Parse timeline events
    timelineEvents.clear();
    if (command.containsKey("events") && command["events"].is<JsonArray>()) {
        JsonArray events = command["events"].as<JsonArray>();
        for (JsonObject event : events) {
            TimelineEvent timelineEvent;
            timelineEvent.time = event.containsKey("time") ? event["time"].as<unsigned long>() : 0;
            timelineEvent.action = event.containsKey("action") ? event["action"].as<String>() : "";
            timelineEvent.executed = false;
            
            // Store params as JSON string for later parsing
            if (event.containsKey("params") && event["params"].is<JsonObject>()) {
                JsonObject params = event["params"].as<JsonObject>();
                String paramsStr;
                serializeJson(params, paramsStr);
                timelineEvent.paramsJson = paramsStr;
            } else {
                timelineEvent.paramsJson = "{}";
            }
            
            timelineEvents.push_back(timelineEvent);
        }
    }
    
    // Get choreography duration
    choreographyDuration = command.containsKey("duration") ? 
        command["duration"].as<unsigned long>() : 0;
    
    LOG_DEBUGF_COMPONENT("ChoreographyManager", 
        "Started choreography with %d beats, %d events, duration %lu ms",
        beatPatterns.size(), timelineEvents.size(), choreographyDuration);
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
    
    // Update timeline events (one-off actions)
    updateTimelineEvents();
    
    // Update beat patterns
    updateBeatPatterns();
    
    // Update active ring players
    if (ringPlayersInitialized) {
        for (auto& rp : ringPlayerPool) {
            if (rp.isPlaying) {
                rp.update(dt);
            }
        }
    }
}

void ChoreographyManager::render(Light* outputBuffer, int numLEDs, int gridRows, int gridCols) {
    // Store buffer and grid dimensions for ring player initialization
    if (!ringPlayersInitialized && outputBuffer != nullptr) {
        initializeRingPlayers(outputBuffer, gridRows, gridCols);
    }
    
    // Ring players write directly to the buffer during update(), so nothing to do here
    (void)numLEDs;
}

void ChoreographyManager::stop() {
    if (!active) return;
    
    active = false;
    
    // Stop all beat patterns
    for (auto& beat : beatPatterns) {
        beat.active = false;
    }
    
    // Stop all active ring players
    if (ringPlayersInitialized) {
        for (auto& rp : ringPlayerPool) {
            if (rp.isPlaying) {
                rp.isPlaying = false;
            }
        }
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
            // Check if end time reached (endTime of 0 means pattern never ends)
            if (beat.endTime > 0 && elapsed >= beat.endTime) {
                beat.active = false;
                // Don't restore brightness here - let the pulse cycle complete naturally
                // If a new pattern starts, it will override anyway
                continue;
            }
            
            // Check if it's time for next beat
            // BrightnessController handles the full pulse cycle, we just need to check if it's done
            unsigned long interval = beat.getIntervalMs();
            unsigned long timeSinceLastBeat = now - beat.lastBeatTime;
            
            // Trigger next action if interval has passed - timeline always takes precedence
            if (timeSinceLastBeat >= interval) {
                executeBeatAction(beat);
                beat.lastBeatTime = now;
            }
        }
    }
}

void ChoreographyManager::executeBeatAction(BeatPattern& beat) {
    // Parse params JSON string
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, beat.paramsJson);
    if (error) {
        LOG_ERRORF_COMPONENT("ChoreographyManager", 
            "Failed to parse params JSON for beat %s: %s", beat.id.c_str(), error.c_str());
        return;
    }
    
    JsonObject params = doc.as<JsonObject>();
    
    // Dispatch to appropriate action handler
    if (beat.action == "brightness_pulse") {
        executeBrightnessPulse(params);
    } else if (beat.action == "fire_ring") {
        executeFireRing(params);
    } else if (beat.action == "fire_pulse") {
        // TODO: Implement fire_pulse action
        LOG_WARNF_COMPONENT("ChoreographyManager", "fire_pulse action not yet implemented for beat %s", beat.id.c_str());
    } else {
        LOG_ERRORF_COMPONENT("ChoreographyManager", "Unknown action type: %s for beat %s", beat.action.c_str(), beat.id.c_str());
    }
}

void ChoreographyManager::executeBrightnessPulse(const JsonObject& params) {
    BrightnessController* bc = BrightnessController::getInstance();
    if (!bc) {
        LOG_ERROR_COMPONENT("ChoreographyManager", "BrightnessController not available");
        return;
    }
    
    // Parse brightness pulse parameters
    int baseBrightness = params.containsKey("base") ? params["base"].as<int>() : 128;
    int peakBrightness = params.containsKey("peak") ? params["peak"].as<int>() : 255;
    unsigned long pulseDuration = params.containsKey("pulse_duration") ? params["pulse_duration"].as<unsigned long>() : 250;
    
    // Let BrightnessController handle the full pulse cycle (base -> peak -> base)
    bc->startPulseCycle(baseBrightness, peakBrightness, pulseDuration);
    
    LOG_DEBUGF_COMPONENT("ChoreographyManager", 
        "Executing brightness pulse cycle: base=%d, peak=%d, duration=%lu ms",
        baseBrightness, peakBrightness, pulseDuration);
}

void ChoreographyManager::updateTimelineEvents() {
    if (!active) return;
    
    unsigned long elapsed = millis() - choreographyStartTime;
    
    for (auto& event : timelineEvents) {
        // Fire event once when time is reached (never early, may fire late)
        if (!event.executed && elapsed >= event.time) {
            executeEventAction(event);
            event.executed = true;
        }
    }
}

void ChoreographyManager::executeEventAction(TimelineEvent& event) {
    // Parse params JSON string
    DynamicJsonDocument doc(1024);  // Larger size for effect definitions
    DeserializationError error = deserializeJson(doc, event.paramsJson);
    if (error) {
        LOG_ERRORF_COMPONENT("ChoreographyManager", 
            "Failed to parse params JSON for event at %lu ms: %s", event.time, error.c_str());
        return;
    }
    
    JsonObject params = doc.as<JsonObject>();
    
    // Dispatch to appropriate action handler
    if (event.action == "change_effect") {
        executeChangeEffect(params);
    } else if (event.action == "set_brightness") {
        executeSetBrightness(params);
    } else if (event.action == "fire_ring") {
        executeFireRing(params);  // Reuse existing method
    } else if (event.action == "fire_pulse") {
        executeFirePulse(params);
    } else {
        LOG_ERRORF_COMPONENT("ChoreographyManager", 
            "Unknown event action type: %s at time %lu ms", event.action.c_str(), event.time);
    }
}

void ChoreographyManager::executeChangeEffect(const JsonObject& params) {
    if (!effectManager) {
        LOG_ERROR_COMPONENT("ChoreographyManager", "EffectManager not available");
        return;
    }
    
    if (!params.containsKey("effect")) {
        LOG_ERROR_COMPONENT("ChoreographyManager", "change_effect action missing 'effect' parameter");
        return;
    }
    
    JsonObject effectObj = params["effect"].as<JsonObject>();
    auto effect = EffectFactory::createEffect(effectObj);
    if (effect) {
        effectManager->removeAllEffects();
        extern Light BlendLightArr[];
        effectManager->addEffect(std::move(effect), BlendLightArr, NUM_LEDS);
        
        LOG_DEBUGF_COMPONENT("ChoreographyManager", "Changed background effect via timeline event");
    } else {
        LOG_ERROR_COMPONENT("ChoreographyManager", "Failed to create effect from timeline event");
    }
}

void ChoreographyManager::executeSetBrightness(const JsonObject& params) {
    BrightnessController* bc = BrightnessController::getInstance();
    if (!bc) {
        LOG_ERROR_COMPONENT("ChoreographyManager", "BrightnessController not available");
        return;
    }
    
    int brightness = params.containsKey("brightness") ? params["brightness"].as<int>() : 128;
    brightness = constrain(brightness, 0, 255);
    
    bc->setBrightness(brightness);
    
    LOG_DEBUGF_COMPONENT("ChoreographyManager", 
        "Set brightness to %d via timeline event", brightness);
}

void ChoreographyManager::executeFirePulse(const JsonObject& params) {
    // TODO: Implement pulse player firing
    // This will need:
    // 1. A pool of PulsePlayer objects (similar to RingPlayer pool)
    // 2. Parse params (position, color, pulse properties)
    // 3. Find an available PulsePlayer from the pool
    // 4. Configure and start it
    
    String paramsStr;
    serializeJson(params, paramsStr);
    LOG_DEBUGF_COMPONENT("ChoreographyManager", 
        "fire_pulse action not yet implemented (params: %s)", paramsStr.c_str());
}

void ChoreographyManager::executeFireRing(const JsonObject& params) {
    if (!ringPlayersInitialized) {
        LOG_WARN_COMPONENT("ChoreographyManager", "Ring players not initialized - cannot fire ring");
        return;
    }
    
    // Find an available ring player
    RingPlayer* rp = findAvailableRingPlayer();
    if (!rp) {
        LOG_WARN_COMPONENT("ChoreographyManager", "No available ring player in pool - all are playing");
        return;
    }
    
    // Parse position
    float row = params.containsKey("row") ? params["row"].as<float>() : 16.0f;
    float col = params.containsKey("col") ? params["col"].as<float>() : 16.0f;
    
    // Parse colors
    Light hiColor(255, 255, 255); // Default white
    Light loColor(0, 0, 0);       // Default black
    
    if (params.containsKey("hi_color")) {
        String hiColorStr = params["hi_color"].as<String>();
        parseColorString(hiColorStr, hiColor);
    }
    
    if (params.containsKey("lo_color")) {
        String loColorStr = params["lo_color"].as<String>();
        parseColorString(loColorStr, loColor);
    }
    
    // Parse ring properties
    float ringSpeed = params.containsKey("ring_speed") ? params["ring_speed"].as<float>() : 100.0f;
    float ringWidth = params.containsKey("ring_width") ? params["ring_width"].as<float>() : 2.0f;
    float fadeRadius = params.containsKey("fade_radius") ? params["fade_radius"].as<float>() : 50.0f;
    float fadeWidth = params.containsKey("fade_width") ? params["fade_width"].as<float>() : 4.0f;
    float amplitude = params.containsKey("amplitude") ? params["amplitude"].as<float>() : 1.0f;
    bool onePulse = params.containsKey("one_pulse") ? params["one_pulse"].as<bool>() : true;
    
    // Configure ring player
    rp->setRingCenter(row, col);
    rp->setRingProps(ringSpeed, ringWidth, fadeRadius, fadeWidth);
    rp->hiLt = hiColor;
    rp->loLt = loColor;
    rp->Amp = amplitude;
    rp->onePulse = onePulse;
    
    // Start the ring
    rp->Start();
    
    LOG_DEBUGF_COMPONENT("ChoreographyManager", 
        "Fired ring at (%.1f, %.1f) with speed=%.1f, width=%.1f, onePulse=%d",
        row, col, ringSpeed, ringWidth, onePulse ? 1 : 0);
}

void ChoreographyManager::initializeRingPlayers(Light* buffer, int rows, int cols) {
    if (ringPlayersInitialized) return;
    
    outputBuffer = buffer;
    gridRows = rows;
    gridCols = cols;
    
    for (auto& rp : ringPlayerPool) {
        rp.initToGrid(buffer, rows, cols);
    }
    
    ringPlayersInitialized = true;
    LOG_DEBUGF_COMPONENT("ChoreographyManager", 
        "Initialized %d ring players with grid %dx%d", RING_PLAYER_POOL_SIZE, rows, cols);
}

RingPlayer* ChoreographyManager::findAvailableRingPlayer() {
    for (auto& rp : ringPlayerPool) {
        if (!rp.isPlaying) {
            return &rp;
        }
    }
    return nullptr;
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
