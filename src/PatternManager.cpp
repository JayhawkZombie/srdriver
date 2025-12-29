#include "PatternManager.h"
#include "../lights/LEDManager.h"
#include "freertos/LogManager.h"
#include "Globals.h"
#include "GlobalState.h"
#include <ArduinoJson.h>
#include "controllers/BrightnessController.h"

// Global LED manager instance
LEDManager* g_ledManager = nullptr;

// Global variables that other parts of the system expect
Light LightArr[NUM_LEDS];
Light BlendLightArr[NUM_LEDS];
// Light FinalLeds[NUM_LEDS];

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

    // Parse JSON
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, jsonCommand);
    
    if (error) {
        LOG_ERRORF_COMPONENT("PatternManager", "JSON parse failed: %s", error.c_str());
		return;
	}

    // Route to LED manager
    g_ledManager->handleCommand(doc.as<JsonObject>());
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