#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include <array>
#include "../lights/Light.h"
#include "DeviceState.h"
#include "hal/input/buttons/Button.hpp"
#include "../lights/LightPlayer2.h"
#include "../lights/WavePlayer.h"
#include "../lights/DataPlayer.h"
#include <ArduinoBLE.h>
#include <vector>

extern int currentWavePlayerIndex;
extern int currentPatternIndex;
extern std::array<patternData, 40> lp2Data;
extern std::array<LightPlayer2, 40> firedPatternPlayers;
extern WavePlayerConfig wavePlayerConfigs[10];
extern Light LightArr[NUM_LEDS];
extern Button pushButton;
extern Button pushButtonSecondary;
// extern float wavePlayerSpeeds[];
extern std::vector<float> wavePlayerSpeeds;
extern int wavePlayerLengths[];
extern DataPlayer dp;
constexpr int numWavePlayerConfigs = 10;

// Externs for pattern logic
extern void initWaveData(WavePlayerConfig &wp);
extern void initWaveData2(WavePlayerConfig &wp);
extern void initWaveData3(WavePlayerConfig &wp);
extern void initWaveData4(WavePlayerConfig &wp);
extern void initWaveData5(WavePlayerConfig &wp);
extern void initWaveData6(WavePlayerConfig &wp);
extern void initWaveData7(WavePlayerConfig &wp);
extern void initWaveData8(WavePlayerConfig &wp);
extern void initWaveData9(WavePlayerConfig &wp);
extern void initWaveData10(WavePlayerConfig &wp);
extern void UpdatePattern();

extern CRGB leds[];
extern float speedMultiplier;
extern int sharedCurrentIndexState;
extern void DrawError(const CRGB&);
extern void UpdateAllCharacteristicsForCurrentPattern();
extern void IncrementSharedCurrentIndexState(unsigned int limit, unsigned int count);

extern void Pattern_Setup();
extern void Pattern_Loop();
extern void GoToPattern(int patternIndex);
extern void GoToNextPattern();
extern void SwitchWavePlayerIndex(int index);
extern void UpdateCurrentPatternColors(Light newHighLt, Light newLowLt);
extern std::pair<Light, Light> GetCurrentPatternColors();
extern WavePlayer* GetCurrentWavePlayer();
extern void FirePatternFromBLE(int idx, Light on, Light off);

// New functions moved from main.cpp
extern void UpdateColorFromCharacteristic(BLEStringCharacteristic &characteristic, Light &color, bool isHighColor);
extern void UpdateSeriesCoefficientsFromCharacteristic(BLEStringCharacteristic &characteristic, WavePlayer &wp);
extern Light ParseColor(const String &colorStr);
extern void ParseFirePatternCommand(const String &command);
extern void ParseAndExecuteCommand(const String &command);
extern void StartBrightnessPulse(int targetBrightness, unsigned long duration);
extern void StartBrightnessFade(int targetBrightness, unsigned long duration);
extern void UpdateBrightnessPulse();
extern void UpdateBrightnessInt(int value);
extern void UpdateBrightness(float value);

unsigned int findAvailablePatternPlayer();
extern void ApplyFromUserPreferences(DeviceState &state, bool skipBrightness = false);
extern void SaveUserPreferences(const DeviceState &state);