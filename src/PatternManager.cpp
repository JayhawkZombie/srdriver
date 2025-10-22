#include "PatternManager.h"
#include "../lights/LEDManager.h"
#include "freertos/LogManager.h"
#include "Globals.h"
#include "GlobalState.h"
#include <ArduinoJson.h>

// Global LED manager instance
LEDManager* g_ledManager = nullptr;

// Global variables that other parts of the system expect
Light LightArr[NUM_LEDS];
Light BlendLightArr[NUM_LEDS];
Light FinalLeds[NUM_LEDS];

// Legacy functions that other parts of the system expect
void SetAlertWavePlayer(String reason) {
    LOG_DEBUG("SetAlertWavePlayer called: " + reason);
    // Push emergency state onto stack
    if (g_ledManager) {
        g_ledManager->pushState(LEDManagerState::EMERGENCY);
    }
}

void StopAlertWavePlayer(String reason) {
    LOG_DEBUG("StopAlertWavePlayer called: " + reason);
    // Only pop emergency state if we're actually in emergency
    if (g_ledManager) {
        g_ledManager->popState(LEDManagerState::EMERGENCY);
    }
}

void UpdateBrightnessInt(int value) {
    LOG_DEBUG("UpdateBrightnessInt called: " + String(value));
    // Forward to LEDManager
    if (g_ledManager) {
        g_ledManager->setBrightness(value);
    }
}

// Additional legacy functions that main.cpp expects
void SaveUserPreferences(const DeviceState& state) {
    LOG_DEBUG("SaveUserPreferences called");
    LOG_DEBUGF("Saving WiFi credentials - SSID: '%s', Password length: %d", 
              state.wifiSSID.c_str(), state.wifiPassword.length());
    
    // Check if we're in startup mode - skip saving to preserve saved effect data
    // if (isBooting) {
    //     LOG_DEBUG("Skipping preferences save during startup to preserve saved effect data");
    //     return;
    // }
    
    // Use the global preferences manager to save
    prefsManager.begin();
    prefsManager.save(state);
    prefsManager.end();
    
    LOG_DEBUG("User preferences saved successfully");
}

void ApplyFromUserPreferences(DeviceState& state, bool skipBrightness) {
    LOG_DEBUG("ApplyFromUserPreferences called, skipBrightness: " + String(skipBrightness));
    LOG_DEBUGF("Applying preferences - WiFi SSID: '%s' (length: %d), Password length: %d", 
              state.wifiSSID.c_str(), state.wifiSSID.length(), state.wifiPassword.length());
    LOG_DEBUGF("Saved effect - Type: '%s' (length: %d), Params: '%s' (length: %d)", 
              state.currentEffectType.c_str(), state.currentEffectType.length(),
              state.currentEffectParams.c_str(), state.currentEffectParams.length());
    
    // Apply brightness if not skipping
    if (!skipBrightness && g_ledManager) {
        g_ledManager->setBrightness(state.brightness);
        LOG_DEBUGF("Applied brightness: %d", state.brightness);
    }
    
    // Restore saved effect if available
    LOG_DEBUGF("Checking effect restoration - currentEffectType.length(): %d, g_ledManager: %p", 
              state.currentEffectType.length(), g_ledManager);
    
    if (state.currentEffectType.length() > 0 && g_ledManager) {
        LOG_DEBUGF("Restoring saved effect - Type: %s, Params: %s", 
                  state.currentEffectType.c_str(), state.currentEffectParams.c_str());
        
        // Create JSON command to restore the effect
        String effectCommand = "{\"t\":\"effect\",\"e\":{\"t\":\"" + state.currentEffectType + "\"";
        
        if (state.currentEffectParams.length() > 0) {
            effectCommand += ",\"p\":" + state.currentEffectParams;
        }
        
        effectCommand += "}}";
        
        LOG_DEBUG("Restoring effect with command: " + effectCommand);
        
        // Parse and execute the effect command
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, effectCommand);
        
        if (!error) {
            JsonObject command = doc.as<JsonObject>();
            g_ledManager->handleCommand(command);
            LOG_DEBUG("Successfully restored saved effect");
        } else {
            LOG_ERROR("Failed to parse saved effect command: " + String(error.c_str()));
        }
    } else {
        LOG_DEBUG("Effect restoration skipped - either no saved effect or LEDManager not available");
    }
    
    // Apply other settings as needed
    LOG_DEBUG("User preferences applied successfully");
}

// Additional functions that BLEManager expects
WavePlayer* GetCurrentWavePlayer() {
    LOG_DEBUG("GetCurrentWavePlayer called");
    // TODO: Return current wave player
    // For now, return nullptr
    return nullptr;
}

void UpdateColorFromCharacteristic(BLEStringCharacteristic& characteristic, CRGB& color, bool isHighColor) {
    LOG_DEBUG("UpdateColorFromCharacteristic called, isHighColor: " + String(isHighColor));
    // TODO: Implement color update
    // For now, just log it
}

void UpdateSeriesCoefficientsFromCharacteristic(BLEStringCharacteristic& characteristic, WavePlayer& wp) {
    LOG_DEBUG("UpdateSeriesCoefficientsFromCharacteristic called");
    // TODO: Implement series coefficients update
    // For now, just log it
}

void ParseAndExecuteCommand(const String& command) {
    LOG_DEBUG("ParseAndExecuteCommand called: " + command);
    // The command is already a JSON string, so pass it directly
    HandleJSONCommand(command);
}

// Global variable that SpeedController expects
float speedMultiplier = 4.0f;

void Pattern_Setup() {
    LOG_DEBUG("Pattern setup: Initializing LED management system");
    Serial.println("[DEBUG] Pattern_Setup called - creating LEDManager");
    
    // Create LED manager
    g_ledManager = new LEDManager();
    Serial.println("[DEBUG] LEDManager created");
    
    // Start with white LEDs for testing - use pushState to make it persistent
    g_ledManager->pushState(LEDManagerState::EFFECT_PLAYING);
    Serial.println("[DEBUG] Pushed EFFECT_PLAYING state");
    
    LOG_DEBUG("Pattern setup: LED management system initialized with white LEDs");
    Serial.println("[DEBUG] Pattern setup complete - should see white LEDs now");
}

void Pattern_Loop() {
	static unsigned long lastUpdateTime = micros();
	const auto now = micros();
	const auto dt = now - lastUpdateTime;
	float dtSeconds = dt * 0.0000001f;
	
	if (dt < 0) {
        dtSeconds = 0.0016f;  // Handle micros() overflow
	}
	lastUpdateTime = now;

    // Update LED manager
    if (g_ledManager) {
        g_ledManager->update(dtSeconds);
        g_ledManager->render(LightArr, NUM_LEDS);
    }
}

void HandleJSONCommand(const String& jsonCommand) {
    LOG_DEBUG("Handling JSON command: " + jsonCommand);
    
    if (!g_ledManager) {
        LOG_ERROR("LED manager not initialized");
		return;
	}

    // Parse JSON
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, jsonCommand);
    
    if (error) {
        LOG_ERROR("JSON parse failed: " + String(error.c_str()));
		return;
	}

    // Route to LED manager
    g_ledManager->handleCommand(doc.as<JsonObject>());
}

// Legacy compatibility functions (for BLE manager)
void GoToPattern(int patternIndex) {
    LOG_DEBUG("Legacy GoToPattern called with index: " + String(patternIndex));
    // Convert to JSON command - for now, just show white LEDs
    String jsonCommand = "{\"type\":\"effect\",\"effect\":{\"type\":\"white\",\"parameters\":{\"pattern\":" + String(patternIndex) + "}}}";
    HandleJSONCommand(jsonCommand);
}

void UpdateCurrentPatternColors(Light newHighLt, Light newLowLt) {
    LOG_DEBUG("Legacy UpdateCurrentPatternColors called");
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
    LOG_DEBUG("Legacy FirePatternFromBLE called with index: " + String(idx));
    // Convert to JSON command
    String jsonCommand = "{\"type\":\"effect\",\"effect\":{\"type\":\"fire_pattern\",\"parameters\":{\"pattern\":" + String(idx) + ",\"on_color\":[" + 
        String(on.r) + "," + String(on.g) + "," + String(on.b) + "],\"off_color\":[" + 
        String(off.r) + "," + String(off.g) + "," + String(off.b) + "]}}}";
    HandleJSONCommand(jsonCommand);
}