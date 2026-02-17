#include "ChoreographyManager.h"
#include "effects/EffectManager.h"
#include "effects/EffectFactory.h"
#include "../PatternManager.h"
#include "../UserPreferences.h"
#include "../Globals.h"
#include "LEDManager.h"
#include "RingPlayer.h"
#include "PulsePlayer.h"

// Forward declaration for parseColorString (defined in EffectFactory.cpp)
void parseColorString(const String& colorString, Light& color);

// Helper function to parse time strings like "0:45.500" (minutes:seconds.milliseconds) to milliseconds
// Also supports "45.500" (seconds.milliseconds), "1:23" (minutes:seconds), or plain numbers (milliseconds)
unsigned long ChoreographyManager::parseTimeString(const JsonVariant& timeValue) {
    // If it's already a number, return it directly (backward compatibility)
    if (timeValue.is<unsigned long>()) {
        return timeValue.as<unsigned long>();
    }
    if (timeValue.is<long>()) {
        return (unsigned long)timeValue.as<long>();
    }
    if (timeValue.is<int>()) {
        return (unsigned long)timeValue.as<int>();
    }
    
    // If it's a string, parse the time format
    // Format: "M:SS.mmm" (minutes:seconds.milliseconds) or "SS.mmm" (seconds.milliseconds)
    // Example: "0:10.520" = 0 minutes, 10 seconds, 520 milliseconds = 10520 ms
    if (timeValue.is<String>()) {
        String timeStr = timeValue.as<String>();
        timeStr.trim();
        
        unsigned long totalMs = 0;
        
        // Check if it contains a colon (minutes:seconds.milliseconds format)
        int colonIndex = timeStr.indexOf(':');
        if (colonIndex >= 0) {
            // Has minutes:seconds.milliseconds format
            String minutesStr = timeStr.substring(0, colonIndex);
            String secondsAndMsStr = timeStr.substring(colonIndex + 1);
            
            unsigned long minutes = minutesStr.toInt();
            
            // Parse seconds and milliseconds from "SS.mmm"
            int dotIndex = secondsAndMsStr.indexOf('.');
            if (dotIndex >= 0) {
                String secondsStr = secondsAndMsStr.substring(0, dotIndex);
                String msStr = secondsAndMsStr.substring(dotIndex + 1);
                unsigned long seconds = secondsStr.toInt();
                unsigned long milliseconds = msStr.toInt();
                totalMs = (minutes * 60 * 1000) + (seconds * 1000) + milliseconds;
            } else {
                // No milliseconds, just seconds
                unsigned long seconds = secondsAndMsStr.toInt();
                totalMs = (minutes * 60 * 1000) + (seconds * 1000);
            }
        } else {
            // Just seconds.milliseconds format (no minutes)
            int dotIndex = timeStr.indexOf('.');
            if (dotIndex >= 0) {
                String secondsStr = timeStr.substring(0, dotIndex);
                String msStr = timeStr.substring(dotIndex + 1);
                unsigned long seconds = secondsStr.toInt();
                unsigned long milliseconds = msStr.toInt();
                totalMs = (seconds * 1000) + milliseconds;
            } else {
                // Just seconds, no milliseconds
                unsigned long seconds = timeStr.toInt();
                totalMs = seconds * 1000;
            }
        }
        
        return totalMs;
    }
    
    // Default to 0 if we can't parse it
    LOG_WARNF_COMPONENT("ChoreographyManager", "Could not parse time value, defaulting to 0");
    return 0;
}

ChoreographyManager::ChoreographyManager() 
    : active(false), effectManager(nullptr), choreographyStartTime(0), choreographyDuration(0),
      outputBuffer(nullptr), gridRows(32), gridCols(32), ringPlayersInitialized(false),
      numLEDs(0), pulsePlayersInitialized(false), nextPulsePlayerIdx(0),
      lastCountInPulseTime(0) {
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
    lastCountInPulseTime = 0;  // Reset count-in pulse tracking
    
    // Reset ring players for new choreography
    if (ringPlayersInitialized) {
        for (auto& rp : ringPlayerPool) {
            rp.isPlaying = false;
        }
    }
    
    // Save current state
    saveCurrentState();
    
    // Optional top-level param_defs: map of name -> params object for compact "params": "name" in beats/events
    JsonObject paramDefs = command.containsKey("param_defs") && command["param_defs"].is<JsonObject>()
        ? command["param_defs"].as<JsonObject>() : JsonObject();
    
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
            
            // Store params as JSON string for later parsing (inline object or name ref from param_defs)
            {
                JsonObject paramsToUse;
                if (beat.containsKey("params")) {
                    if (beat["params"].is<JsonObject>()) {
                        paramsToUse = beat["params"].as<JsonObject>();
                    } else if ((beat["params"].is<const char*>() || beat["params"].is<String>()) && !paramDefs.isNull()) {
                        const char* name = nullptr;
                        String nameStr;
                        if (beat["params"].is<const char*>()) {
                            name = beat["params"].as<const char*>();
                        } else {
                            nameStr = beat["params"].as<String>();
                            name = nameStr.c_str();
                        }
                        paramsToUse = paramDefs[name].as<JsonObject>();
                        if (paramsToUse.isNull()) {
                            LOG_WARNF_COMPONENT("ChoreographyManager", "param_defs[\"%s\"] not found for beat, using {}", name);
                        }
                    }
                }
                String paramsStr;
                if (!paramsToUse.isNull()) {
                    serializeJson(paramsToUse, paramsStr);
                }
                pattern.paramsJson = paramsStr.length() > 0 ? paramsStr : "{}";
            }
            
            // Parse start_t - supports both number (ms) and string (e.g., "0:45.500")
            pattern.startTime = beat.containsKey("start_t") ? 
                parseTimeString(beat["start_t"]) : 0;
            
            // Support both "duration" and "end_t" - convert duration to endTime if provided
            if (beat.containsKey("duration")) {
                unsigned long duration = parseTimeString(beat["duration"]);
                pattern.endTime = pattern.startTime + duration;
            } else if (beat.containsKey("end_t")) {
                pattern.endTime = parseTimeString(beat["end_t"]);
            } else {
                // Default to 0 if neither provided (pattern never ends)
                pattern.endTime = 0;
            }
            
            pattern.lastBeatTime = 0;
            pattern.active = false;
            
            // Only add non-empty beat patterns (skip empty objects in JSON)
            // Check if the JSON object actually has any keys - if it's empty {}, size() will be 0
            if (beat.size() > 0) {
                beatPatterns.push_back(pattern);
            } else {
                LOG_DEBUG_COMPONENT("ChoreographyManager", "Skipping empty beat pattern object in JSON");
            }
        }
    }
    
    // Parse timeline events
    timelineEvents.clear();
    if (command.containsKey("events") && command["events"].is<JsonArray>()) {
        JsonArray events = command["events"].as<JsonArray>();
        for (JsonObject event : events) {
            TimelineEvent timelineEvent;
            // Parse time - supports both number (ms) and string (e.g., "0:45.500")
            timelineEvent.time = event.containsKey("time") ? 
                parseTimeString(event["time"]) : 0;
            timelineEvent.action = event.containsKey("action") ? event["action"].as<String>() : "";
            timelineEvent.executed = false;
            
            // Store params as JSON string for later parsing (inline object or name ref from param_defs)
            {
                JsonObject paramsToUse;
                if (event.containsKey("params")) {
                    if (event["params"].is<JsonObject>()) {
                        paramsToUse = event["params"].as<JsonObject>();
                    } else if ((event["params"].is<const char*>() || event["params"].is<String>()) && !paramDefs.isNull()) {
                        const char* name = nullptr;
                        String nameStr;
                        if (event["params"].is<const char*>()) {
                            name = event["params"].as<const char*>();
                        } else {
                            nameStr = event["params"].as<String>();
                            name = nameStr.c_str();
                        }
                        paramsToUse = paramDefs[name].as<JsonObject>();
                        if (paramsToUse.isNull()) {
                            LOG_WARNF_COMPONENT("ChoreographyManager", "param_defs[\"%s\"] not found for event, using {}", name);
                        }
                    }
                }
                String paramsStr;
                if (!paramsToUse.isNull()) {
                    serializeJson(paramsToUse, paramsStr);
                }
                timelineEvent.paramsJson = paramsStr.length() > 0 ? paramsStr : "{}";
            }
            
            timelineEvents.push_back(timelineEvent);
        }
    }
    
    // Get choreography duration - supports both number (ms) and string (e.g., "1:06.000")
    choreographyDuration = command.containsKey("duration") ? 
        parseTimeString(command["duration"]) : 0;
    
    LOG_DEBUGF_COMPONENT("ChoreographyManager", 
        "Started choreography with %d beats, %d events, duration %lu ms",
        beatPatterns.size(), timelineEvents.size(), choreographyDuration);
}

void ChoreographyManager::update(float dt) {
    if (!active) return;
    
    unsigned long elapsed = millis() - choreographyStartTime;
    
    // Handle count-in phase (first 3 seconds)
    if (elapsed < COUNT_IN_DURATION) {
        // Fire white ring pulse at 0s, 1s, 2s (once per second)
        unsigned long countInSecond = elapsed / 1000;  // Which second we're in (0, 1, or 2)
        unsigned long expectedPulseTime = countInSecond * 1000;  // Expected time for this pulse
        
        // Fire pulse if we've reached a new second and haven't fired for this second yet
        if (elapsed >= expectedPulseTime && lastCountInPulseTime < expectedPulseTime) {
            // Fire count-in ring (white pulse at center)
            RingPlayer* rp = findAvailableRingPlayer();
            if (rp && ringPlayersInitialized) {
                rp->setRingCenter(16.0f, 16.0f);
                rp->setRingProps(20.0f, 6.0f, 12.0f, 12.0f);
                rp->hiLt = Light(255, 255, 255);  // White
                rp->loLt = Light(0, 0, 0);        // Black
                rp->Amp = 1.0f;
                rp->onePulse = true;
                rp->Start();
                lastCountInPulseTime = expectedPulseTime;
            }
        }
        
        // Update active ring players during count-in
        if (ringPlayersInitialized) {
            for (auto& rp : ringPlayerPool) {
                if (rp.isPlaying) {
                    rp.update(dt);
                }
            }
        }
        if (pulsePlayersInitialized) {
            for (auto& pp : pulsePlayerPool) {
                pp.update(dt);
            }
        }
        return;  // Don't process timeline during count-in
    }
    
    // After count-in, calculate timeline-relative elapsed time
    unsigned long timelineElapsed = elapsed - COUNT_IN_DURATION;
    
    // Check if choreography is complete - stop when duration is reached
    if (choreographyDuration > 0 && timelineElapsed >= choreographyDuration) {
        stop();
        return;
    }
    
    // Update timeline events (one-off actions)
    updateTimelineEvents(timelineElapsed);
    
    // Update beat patterns
    updateBeatPatterns(timelineElapsed);
    
    // Update active ring players
    if (ringPlayersInitialized) {
        for (auto& rp : ringPlayerPool) {
            if (rp.isPlaying) {
                rp.update(dt);
            }
        }
    }
    // Update all pulse players (round-robin pool; each no-ops when past strip)
    if (pulsePlayersInitialized) {
        for (auto& pp : pulsePlayerPool) {
            pp.update(dt);
        }
    }
}

void ChoreographyManager::render(Light* outputBuffer, int numLEDs, int gridRows, int gridCols) {
    // Store buffer and grid dimensions for ring player initialization
    if (!ringPlayersInitialized && outputBuffer != nullptr) {
        initializeRingPlayers(outputBuffer, gridRows, gridCols);
    }
    // Pulse players use 1D strip; initialize when we have buffer and LED count
    if (!pulsePlayersInitialized && outputBuffer != nullptr && numLEDs > 0) {
        this->numLEDs = numLEDs;
        initializePulsePlayers(outputBuffer, numLEDs);
    }
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

void ChoreographyManager::updateBeatPatterns(unsigned long timelineElapsed) {
    unsigned long now = millis();
    
    BrightnessController* bc = BrightnessController::getInstance();
    
    for (auto& beat : beatPatterns) {
        // Check if beat pattern should start (using timeline-relative time)
        if (!beat.active && timelineElapsed >= beat.startTime) {
            beat.active = true;
            beat.lastBeatTime = now;
        }
        
        // Update active beat patterns
        if (beat.active) {
            // Check if end time reached (endTime of 0 means pattern never ends)
            // Use timeline-relative time for comparison
            if (beat.endTime > 0 && timelineElapsed >= beat.endTime) {
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
        executeFirePulse(params);
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

void ChoreographyManager::updateTimelineEvents(unsigned long timelineElapsed) {
    if (!active) return;
    
    for (auto& event : timelineEvents) {
        // Fire event once when time is reached (never early, may fire late)
        // Use timeline-relative time for comparison
        if (!event.executed && timelineElapsed >= event.time) {
            executeEventAction(event);
            event.executed = true;
        }
    }
}

void ChoreographyManager::executeEventAction(TimelineEvent& event) {
    // Parse params JSON string (e.g. change_effect with nested effect can be large)
    DynamicJsonDocument doc(2048);
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
    } else if (event.action == "update_effect_params") {
        executeUpdateEffectParams(params);
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

void ChoreographyManager::executeUpdateEffectParams(const JsonObject& params) {
    LOG_DEBUGF_COMPONENT("ChoreographyManager", "Executing update_effect_params");
    if (!effectManager) {
        LOG_ERROR_COMPONENT("ChoreographyManager", "EffectManager not available");
        return;
    }
    Effect* effect = effectManager->getPrimaryEffect();
    if (!effect) {
        LOG_WARN_COMPONENT("ChoreographyManager", "update_effect_params: no active effect");
        return;
    }
    if (!effect->updateParams(params)) {
        LOG_DEBUGF_COMPONENT("ChoreographyManager", "Current effect does not support updateParams");
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
    if (!pulsePlayersInitialized || !outputBuffer || numLEDs <= 0) {
        LOG_WARN_COMPONENT("ChoreographyManager", "Pulse players not initialized - cannot fire pulse");
        return;
    }

    // Round-robin: use next slot in the pool
    PulsePlayer* pp = &pulsePlayerPool[nextPulsePlayerIdx];
    nextPulsePlayerIdx = (nextPulsePlayerIdx + 1) % PULSE_PLAYER_POOL_SIZE;

    // Parse color (default white)
    Light hiColor(255, 255, 255);
    if (params.containsKey("hi_color")) {
        String hiColorStr = params["hi_color"].as<String>();
        parseColorString(hiColorStr, hiColor);
    }

    // Parse pulse properties (match PulsePlayerEffect / EffectFactory naming where applicable)
    int pulseWidth = params.containsKey("pulse_width") ? params["pulse_width"].as<int>() : 16;
    if (pulseWidth < 1) pulseWidth = 1;
    float speed = params.containsKey("speed") ? params["speed"].as<float>() : 50.0f;
    bool reverse = params.containsKey("reverse") ? params["reverse"].as<bool>() : false;
    if (reverse) speed = -speed;

    pp->init(outputBuffer[0], numLEDs, hiColor, pulseWidth, speed, false);
    pp->Start();

    int slot = (nextPulsePlayerIdx + PULSE_PLAYER_POOL_SIZE - 1) % PULSE_PLAYER_POOL_SIZE;
    // LOG_DEBUGF_COMPONENT("ChoreographyManager",
    //     "Fired pulse (round-robin slot %d): width=%d, speed=%.1f", slot, pulseWidth, speed);
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
    
    // LOG_DEBUGF_COMPONENT("ChoreographyManager", 
    //     "Fired ring at (%.1f, %.1f) with speed=%.1f, width=%.1f, onePulse=%d",
    //     row, col, ringSpeed, ringWidth, onePulse ? 1 : 0);
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

void ChoreographyManager::initializePulsePlayers(Light* buffer, int numLeds) {
    if (pulsePlayersInitialized) return;

    outputBuffer = buffer;
    numLEDs = numLeds;
    for (auto& pp : pulsePlayerPool) {
        // Park each player off-strip (doRepeat=false, no Start) so they're ready for round-robin fire
        pp.init(buffer[0], numLeds, Light(0, 0, 0), 1, 1.0f, false);
    }
    pulsePlayersInitialized = true;
    LOG_DEBUGF_COMPONENT("ChoreographyManager",
        "Initialized %d pulse players (round-robin) with %d LEDs", PULSE_PLAYER_POOL_SIZE, numLeds);
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
