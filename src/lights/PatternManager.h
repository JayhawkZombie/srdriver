#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Light.h"
#include "../Globals.h"
#include "../DeviceState.h"
#include <ArduinoBLE.h>
#include "players/WavePlayer.h"

// Forward declarations
class LEDManager;

// Global LED manager instance
extern LEDManager* g_ledManager;

// Core pattern functions (required by LED task)
void Pattern_Setup();
void Pattern_Loop();

// JSON command interface
void HandleJSONCommand(const String& jsonCommand);

// Effect list management
void InitializeEffectList(const std::vector<String>& builtInEffects);
void TriggerNextEffect();
bool LoadEffectsFromStorage();
int GetCurrentEffectIndex();
int GetEffectCount();

// Legacy compatibility (for BLE manager)
void GoToPattern(int patternIndex);
void UpdateCurrentPatternColors(Light newHighLt, Light newLowLt);
std::pair<Light, Light> GetCurrentPatternColors();
void FirePatternFromBLE(int idx, Light on, Light off);

// Legacy functions that other parts of the system expect
void SetAlertWavePlayer(String reason);
void StopAlertWavePlayer(String reason);
void UpdateBrightnessInt(int value);
void SaveUserPreferences(const DeviceState& state);
void ApplyFromUserPreferences(DeviceState& state, bool skipBrightness = false);

// Functions that BLEManager expects
WavePlayer* GetCurrentWavePlayer();
void UpdateColorFromCharacteristic(BLEStringCharacteristic& characteristic, CRGB& color, bool isHighColor);
void UpdateSeriesCoefficientsFromCharacteristic(BLEStringCharacteristic& characteristic, WavePlayer& wp);
void ParseAndExecuteCommand(const String& command);

// Global variables that other parts of the system expect
extern Light LightArr[NUM_LEDS];
extern Light BlendLightArr[NUM_LEDS];
extern Light FinalLeds[NUM_LEDS];