#include "PatternManager.h"
#include "../lights/LEDManager.h"
#include "freertos/LogManager.h"
#include "Globals.h"
#include "GlobalState.h"
#include <ArduinoJson.h>
#include "controllers/BrightnessController.h"
#include <vector>
#if SUPPORTS_SD_CARD
#include "hal/SDCardController.h"
#endif

// Global LED manager instance
LEDManager* g_ledManager = nullptr;

// Global variables that other parts of the system expect
Light LightArr[NUM_LEDS];
Light BlendLightArr[NUM_LEDS];
// Light FinalLeds[NUM_LEDS];

// Effect list management (static to PatternManager)
static std::vector<String> effectOrderJsonStrings;
static int currentEffectIndex = 0;

// Legacy functions that other parts of the system expect
void SetAlertWavePlayer(String reason) {
    // Push emergency state onto stack
    if (g_ledManager) {
        g_ledManager->pushState(LEDManagerState::EMERGENCY);
    }
}

void StopAlertWavePlayer(String reason) {
    // Only pop emergency state if we're actually in emergency
    if (g_ledManager) {
        g_ledManager->popState(LEDManagerState::EMERGENCY);
    }
}

void UpdateBrightnessInt(int value) {
    // Forward to LEDManager
    if (g_ledManager) {
        g_ledManager->setBrightness(value);
    }
}

// Additional legacy functions that main.cpp expects
void SaveUserPreferences(const DeviceState& state) {
    // Check if we're in startup mode - skip saving to preserve saved effect data
    // if (isBooting) {
    //     LOG_DEBUG("Skipping preferences save during startup to preserve saved effect data");
    //     return;
    // }
    
    // Use the global preferences manager to save
    prefsManager.begin();
    prefsManager.save(state);
    prefsManager.end();
}

void ApplyFromUserPreferences(DeviceState& state, bool skipBrightness) {
    // Apply brightness if not skipping
    if (!skipBrightness && g_ledManager) {
        LOG_DEBUGF_COMPONENT("PatternManager", "ApplyFromUserPreferences: state.brightness=%d", state.brightness);
        g_ledManager->setBrightness(state.brightness);
        // Update brightness controller?
        BrightnessController* brightnessController = BrightnessController::getInstance();
        if (brightnessController) {
            brightnessController->setBrightness(state.brightness);
        }
    }
    
    // Restore saved effect if available
    if (state.currentEffectType.length() > 0 && g_ledManager) {
        // Create JSON command to restore the effect
        String effectCommand = "{\"t\":\"effect\",\"e\":{\"t\":\"" + state.currentEffectType + "\"";
        
        if (state.currentEffectParams.length() > 0) {
            effectCommand += ",\"p\":" + state.currentEffectParams;
        }
        
        effectCommand += "}}";
        
        // Parse and execute the effect command
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, effectCommand);
        
        if (!error) {
            JsonObject command = doc.as<JsonObject>();
            g_ledManager->handleCommand(command);
        }
    }
}

// Additional functions that BLEManager expects
WavePlayer* GetCurrentWavePlayer() {
    // TODO: Return current wave player
    // For now, return nullptr
    return nullptr;
}

void UpdateColorFromCharacteristic(BLEStringCharacteristic& characteristic, CRGB& color, bool isHighColor) {
    // TODO: Implement color update
    // For now, just log it
}

void UpdateSeriesCoefficientsFromCharacteristic(BLEStringCharacteristic& characteristic, WavePlayer& wp) {
    // TODO: Implement series coefficients update
    // For now, just log it
}

void ParseAndExecuteCommand(const String& command) {
    // The command is already a JSON string, so pass it directly
    HandleJSONCommand(command);
}

// Global variable that SpeedController expects
float speedMultiplier = 4.0f;

void Pattern_Setup() {
    // Create LED manager
    g_ledManager = new LEDManager();
    
    // Start with white LEDs for testing - use pushState to make it persistent
    g_ledManager->pushState(LEDManagerState::EFFECT_PLAYING);
}

// void Pattern_Loop() {
// 	static unsigned long lastUpdateTime = micros();
// 	const auto now = micros();
// 	const auto dt = now - lastUpdateTime;
// 	float dtSeconds = dt * 0.0000001f;
	
// 	if (dt < 0) {
//         dtSeconds = 0.0016f;  // Handle micros() overflow
// 	}
// 	lastUpdateTime = now;

//     // Update LED manager
//     if (g_ledManager) {
//         g_ledManager->safeProcessQueue();
//         g_ledManager->update(dtSeconds);
//         g_ledManager->render(LightArr, NUM_LEDS);
//         // Process thread-safe queue (after update and render)
//     }
// }

void HandleJSONCommand(const String& jsonCommand) {
    if (!g_ledManager) {
        LOG_ERROR_COMPONENT("PatternManager", "LED manager not initialized");
		return;
	}

    // Parse JSON - ArduinoJson needs 2.5-3x the string length for complex structures
    // Use a minimum of 2KB to handle small commands, and cap at 32KB to avoid excessive memory usage
    size_t jsonSize = jsonCommand.length();
    size_t docSize = jsonSize * 3;  // 3x multiplier for safety with complex nested structures
    if (docSize < 2048) {
        docSize = 2048;  // Minimum 2KB for small commands
    }
    if (docSize > 32768) {
        docSize = 32768;  // Cap at 32KB
    }
    
    DynamicJsonDocument doc(docSize);
    DeserializationError error = deserializeJson(doc, jsonCommand);
    
    if (error) {
        LOG_ERRORF_COMPONENT("PatternManager", "JSON parse failed: %s (Code: %d). String length: %d, Doc size: %d", 
                            error.c_str(), error.code(), jsonSize, docSize);
        return;
    }

    LOG_DEBUGF_COMPONENT("PatternManager", "Handling JSON command (string: %d bytes, doc: %d bytes)", jsonSize, docSize);
    // Use queued command if handler supports it (for thread-safe processing)
    // This prevents race conditions when commands come from WebSocket while LED update task is rendering
    if (g_ledManager->supportsQueuing()) {
        // Create a shared_ptr to the document so it can be safely queued
        // Use the same size as the parsed document to ensure copy succeeds
        auto sharedDoc = std::make_shared<DynamicJsonDocument>(docSize);
        *sharedDoc = doc;  // Copy the parsed document
        g_ledManager->handleQueuedCommand(sharedDoc);
    } else {
        // Fall back to direct command handling for handlers that don't support queuing
        g_ledManager->handleCommand(doc.as<JsonObject>());
    }
}

// Legacy compatibility functions (for BLE manager)
void GoToPattern(int patternIndex) {
    // Convert to JSON command - for now, just show white LEDs
    String jsonCommand = "{\"type\":\"effect\",\"effect\":{\"type\":\"white\",\"parameters\":{\"pattern\":" + String(patternIndex) + "}}}";
    HandleJSONCommand(jsonCommand);
}

void UpdateCurrentPatternColors(Light newHighLt, Light newLowLt) {
    // Convert to JSON command
    String jsonCommand = "{\"type\":\"effect\",\"effect\":{\"type\":\"wave_pattern\",\"parameters\":{\"high_color\":[" + 
        String(newHighLt.r) + "," + String(newHighLt.g) + "," + String(newHighLt.b) + "],\"low_color\":[" + 
        String(newLowLt.r) + "," + String(newLowLt.g) + "," + String(newLowLt.b) + "]}}}";
    HandleJSONCommand(jsonCommand);
}

std::pair<Light, Light> GetCurrentPatternColors() {
    // Return default colors for legacy compatibility
    return std::make_pair(Light(255, 255, 255), Light(0, 0, 0));
}

void FirePatternFromBLE(int idx, Light on, Light off) {
    // Convert to JSON command
    String jsonCommand = "{\"type\":\"effect\",\"effect\":{\"type\":\"fire_pattern\",\"parameters\":{\"pattern\":" + String(idx) + ",\"on_color\":[" + 
        String(on.r) + "," + String(on.g) + "," + String(on.b) + "],\"off_color\":[" + 
        String(off.r) + "," + String(off.g) + "," + String(off.b) + "]}}}";
    HandleJSONCommand(jsonCommand);
}

// Effect list management functions
void InitializeEffectList(const std::vector<String>& builtInEffects) {
    effectOrderJsonStrings = builtInEffects;
    currentEffectIndex = 0;
    LOG_INFOF_COMPONENT("PatternManager", "Initialized effect list with %d effects", 
                       effectOrderJsonStrings.size());
}

void TriggerNextEffect() {
    if (effectOrderJsonStrings.size() == 0) {
        LOG_WARN_COMPONENT("PatternManager", "No effects loaded - cannot cycle");
        return;
    }
    
    currentEffectIndex++;
    if (currentEffectIndex >= effectOrderJsonStrings.size()) {
        currentEffectIndex = 0;
    }
    
    String effectCommandJsonString = effectOrderJsonStrings[currentEffectIndex];
    LOG_DEBUGF_COMPONENT("PatternManager", "Cycling to effect %d/%d", 
                        currentEffectIndex + 1, effectOrderJsonStrings.size());
    HandleJSONCommand(effectCommandJsonString);
}

void TriggerChoreography() {
    extern LEDManager* g_ledManager;
    if (!g_ledManager) {
        LOG_WARN_COMPONENT("PatternManager", "LEDManager not available - cannot trigger choreography");
        return;
    }
    
#if SUPPORTS_SD_CARD
    extern SDCardController* g_sdCardController;
    
    if (!g_sdCardController || !g_sdCardController->isAvailable()) {
        LOG_WARN_COMPONENT("PatternManager", "SD card not available - cannot load choreography");
        return;
    }
    
    // Load choreography from file
    String timelineJsonString = g_sdCardController->readFile("/data/music/test_timeline.json");
    if (timelineJsonString.length() == 0) {
        LOG_WARN_COMPONENT("PatternManager", "Timeline file not found or empty: /data/music/test_timeline.json");
        return;
    }
    
    // Calculate required JSON document size (similar to LoadEffectsFromStorage)
    size_t fileSize = timelineJsonString.length();
    size_t docSize = (fileSize * 3);
    if (docSize < 2048) {
        docSize = 2048;  // Minimum 2KB
    }
    if (docSize > 64000) {
        docSize = 64000;  // Cap at 64KB
    }
    
    LOG_DEBUGF_COMPONENT("PatternManager", "Loading timeline file: %d bytes, allocating JSON document: %d bytes", 
                        fileSize, docSize);
    
    DynamicJsonDocument doc(docSize);
    DeserializationError error = deserializeJson(doc, timelineJsonString);
    
    if (error) {
        LOG_ERRORF_COMPONENT("PatternManager", "Failed to deserialize timeline JSON: %s (Code: %d)", 
                            error.c_str(), error.code());
        return;
    }
    
    JsonObject command = doc.as<JsonObject>();
    
    LOG_DEBUG_COMPONENT("PatternManager", "Triggering choreography from file");
    g_ledManager->handleCommand(command);
#else
    LOG_WARN_COMPONENT("PatternManager", "SD card not supported on this platform");
#endif
}

bool LoadEffectsFromStorage() {
#if SUPPORTS_SD_CARD
    extern SDCardController* g_sdCardController;
    
    if (!g_sdCardController || !g_sdCardController->isAvailable()) {
        LOG_DEBUG_COMPONENT("PatternManager", "SD card not available, skipping effect storage read");
        return false;
    }
    
    String effectsJsonString = g_sdCardController->readFile("/data/default_effects.json");
    if (effectsJsonString.length() == 0) {
        LOG_DEBUG_COMPONENT("PatternManager", "Effects file not found or empty");
        return false;
    }
    
    // Calculate required JSON document size
    // ArduinoJson typically needs 1.5-2x the file size for parsing
    // Use 2.5x for safety margin, with a minimum of 8KB
    size_t fileSize = effectsJsonString.length();
    size_t docSize = (fileSize * 2.5f);
    if (docSize < 8192) {
        docSize = 8192;  // Minimum 8KB
    }
    // Cap at 64KB to avoid excessive memory usage
    if (docSize > 65536) {
        docSize = 65536;
    }
    
    LOG_DEBUGF_COMPONENT("PatternManager", "File size: %d bytes, allocating JSON document: %d bytes", 
                        fileSize, docSize);
    
    // Use DynamicJsonDocument with calculated size
    DynamicJsonDocument doc(docSize);
    DeserializationError error = deserializeJson(doc, effectsJsonString);
    
    if (error) {
        LOG_ERRORF_COMPONENT("PatternManager", "Failed to deserialize effects JSON: %s (Code: %d). File size: %d, Doc size: %d", 
                            error.c_str(), error.code(), fileSize, docSize);
        
        // If it's an out-of-memory error, suggest increasing doc size
        if (error == DeserializationError::NoMemory) {
            LOG_ERROR_COMPONENT("PatternManager", "JSON document too small - need more memory");
        }
        return false;
    }
    
    std::vector<String> newEffectOrderJsonStrings;
    if (doc.containsKey("effects")) {
        JsonArray effects = doc["effects"];
        for (JsonVariant effect : effects) {
            String effectJson = effect.as<String>();
            if (effectJson.length() > 0) {
                newEffectOrderJsonStrings.push_back(effectJson);
            }
        }
    }
    
    if (newEffectOrderJsonStrings.size() > 0) {
        effectOrderJsonStrings = newEffectOrderJsonStrings;
        currentEffectIndex = 0;  // Reset to first effect
        LOG_INFOF_COMPONENT("PatternManager", "Loaded %d effects from storage", 
                          effectOrderJsonStrings.size());
        return true;
    }
    
    LOG_WARN_COMPONENT("PatternManager", "No effects found in storage file");
    return false;
#else
    LOG_DEBUG_COMPONENT("PatternManager", "SD card not supported on this platform");
    return false;
#endif
}

int GetCurrentEffectIndex() {
    return currentEffectIndex;
}

int GetEffectCount() {
    return effectOrderJsonStrings.size();
}