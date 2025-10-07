#include "PatternManager.h"
#include <FastLED.h>
#include "Globals.h"
#include "../lights/Light.h"
#include "../lights/LightPanel.h"
#include "../lights/PulsePlayer.h"
#include "../lights/RingPlayer.h"
#include "DeviceState.h"
#include <array>
#include <math.h>
#include "hal/ble/BLEManager.h"
#include "GlobalState.h"
#include "Utils.hpp"
#include "freertos/LogManager.h"
#include "freertos/HardwareInputTask.h"
#include "config/JsonSettings.h"
#include "controllers/BrightnessController.h"
#include "controllers/SpeedController.h"
#include "../lights/blending/LayerStack.h"
#include "../lights/blending/MainLayer.h"
#include "../lights/blending/PatternLayer.h"

// Add externs for all globals and helpers used by pattern logic
unsigned int findAvailablePatternPlayer();
void SetupWavePlayerCoefficients(const WavePlayerConfig &config, float *C_Rt, float *C_Lt, float **rightCoeffs, float **leftCoeffs, int *nTermsRt, int *nTermsLt);

// Pattern-related global definitions
int currentWavePlayerIndex = 0;
int currentPatternIndex = 0;
std::array<patternData, 40> lp2Data;
std::array<LightPlayer2, 40> firedPatternPlayers;
WavePlayerConfig wavePlayerConfigs[10];
Light LightArr[NUM_LEDS];
Light BlendLightArr[NUM_LEDS];
Light FinalLeds[NUM_LEDS];

// LightPanel setup for 2x2 configuration
LightPanel panels[4];  // 4 panels in 2x2 grid

Button pushButton(PUSHBUTTON_PIN);
Button pushButtonSecondary(PUSHBUTTON_PIN_SECONDARY);
bool rainbowPlayerActive = false;
// bool rainbowPlayer2Active = false;
RainbowPlayer rainbowPlayer(LightArr, NUM_LEDS, 0, NUM_LEDS - 1, 1.0f, false);  // Full 32x32 array
// RainbowPlayer rainbowPlayer2(LightArr, NUM_LEDS, NUM_LEDS/2, NUM_LEDS - 1, 1.0f, true); // Second half, reverse direction
// float wavePlayerSpeeds[] = { 0.001f, 0.0035f, 0.003f, 0.001f, 0.001f, 0.0005f, 0.001f, 0.001f, 0.001f, 0.001f };
std::vector<float> wavePlayerSpeeds;
DataPlayer dp;
int sharedCurrentIndexState = 0;
// Global speedMultiplier for backward compatibility with SpeedController
float speedMultiplier = 4.0f;  // Default value, will be updated by SpeedController

WavePlayer alertWavePlayer;
bool alertWavePlayerActive = false;

// Add separate buffer for alert wave player
Light AlertLightArr[NUM_LEDS];

// Layer system
std::unique_ptr<LayerStack> layerStack;

// Forward declarations for BLE manager access
// extern BLEManager bleManager;

// --- Pattern Logic Isolation ---
extern JsonSettings settings;
extern SDCardController *g_sdCardController;
DynamicJsonDocument patternsDoc(8196 * 8);  // Increased from 5 to 8 to handle larger JSON with both configs

WavePlayerConfig jsonWavePlayerConfigs[12];  // Increased from 10 to 12 to accommodate more configs

PulsePlayer pulsePlayer;
// RingPlayer lavenderRingPlayer;
// RingPlayer unusualRingPlayers[1];
// RingPlayer unusualRingPlayers[2], unusualRingPlayers[3];

RingPlayer unusualRingPlayers[4];


int playingRingPlayer = 0;
int maxRingPlayers = 4;

void MoveToNextRingPlayer()
{
	unusualRingPlayers[playingRingPlayer].StopWave();
	playingRingPlayer = (playingRingPlayer + 1) % maxRingPlayers;
	unusualRingPlayers[playingRingPlayer].Start();
}

extern HardwareInputTask *g_hardwareInputTask;


// Try loading a couple from /data/patterns.json
bool LoadPatternsFromJson()
{
	if (g_sdCardController->isAvailable())
	{
		LOG_DEBUG("Loading patterns from /data/patterns.json");

		String patternsJson = g_sdCardController->readFile("/data/patterns.json");
		DeserializationError error = deserializeJson(patternsDoc, patternsJson);
		if (error)
		{
			LOG_ERRORF("Failed to deserialize patterns JSON: %s", error.c_str());
			return false;
		}
		LOG_DEBUG("Patterns loaded successfully");
		return true;
	}
	return false;
}

void LoadWavePlayerConfigsFromJsonDocument()
{
	LOG_DEBUG("Loading wave player configs from JSON document");
	if (patternsDoc.isNull())
	{
		LOG_ERROR("Patterns document is null");
		return;
	}

	JsonArray wavePlayerConfigsArray = patternsDoc["wavePlayerConfigs"];
	if (wavePlayerConfigsArray.isNull())
	{
		LOG_ERROR("Wave player configs array is null");
		return;
	}

	for (int i = 0; i < wavePlayerConfigsArray.size(); i++)
	{
		LOG_DEBUGF("Loading wave player config %d", i);
		const JsonObject &config = wavePlayerConfigsArray[i];
		WavePlayerConfig &wpConfig = jsonWavePlayerConfigs[i];
		wpConfig.name = config["name"].as<String>();
		wpConfig.rows = config["rows"].as<int>();
		wpConfig.cols = config["cols"].as<int>();
		wpConfig.onLight = Light(config["onLight"]["r"].as<int>(), config["onLight"]["g"].as<int>(), config["onLight"]["b"].as<int>());
		wpConfig.offLight = Light(config["offLight"]["r"].as<int>(), config["offLight"]["g"].as<int>(), config["offLight"]["b"].as<int>());
		wpConfig.AmpRt = config["AmpRt"].as<float>();
		wpConfig.wvLenLt = config["wvLenLt"].as<float>();
		wpConfig.wvLenRt = config["wvLenRt"].as<float>();
		wpConfig.wvSpdLt = config["wvSpdLt"].as<float>();
		wpConfig.wvSpdRt = config["wvSpdRt"].as<float>();
		wpConfig.rightTrigFuncIndex = config["rightTrigFuncIndex"].as<int>();
		wpConfig.leftTrigFuncIndex = config["leftTrigFuncIndex"].as<int>();
		wpConfig.useRightCoefficients = config["useRightCoefficients"].as<bool>();
		wpConfig.useLeftCoefficients = config["useLeftCoefficients"].as<bool>();
		wpConfig.nTermsRt = config["nTermsRt"].as<int>();
		wpConfig.nTermsLt = config["nTermsLt"].as<int>();
		wpConfig.speed = config["speed"].as<float>();
		wavePlayerSpeeds.push_back(wpConfig.speed);
		const auto lCoeff = config["C_Lt"];
		const auto rCoeff = config["C_Rt"];

		// if (wpConfig.useRightCoefficients) {
		for (int j = 0; j < 3; j++)
		{
			wpConfig.C_Rt[j] = config["C_Rt"][j].as<float>();
		}
		// }
		// if (wpConfig.useLeftCoefficients) {
		for (int j = 0; j < 3; j++)
		{
			wpConfig.C_Lt[j] = config["C_Lt"][j].as<float>();
		}
		// }
		wpConfig.setCoefficients(wpConfig.C_Rt, wpConfig.C_Lt);
		LOG_DEBUGF("Loaded wave player config %d: %s, %d, %d", i, wpConfig.name.c_str(), wpConfig.nTermsRt, wpConfig.nTermsLt);
		if (wpConfig.useRightCoefficients)
		{
			LOG_DEBUGF("C_Rt: %f, %f, %f", wpConfig.C_Rt[0], wpConfig.C_Rt[1], wpConfig.C_Rt[2]);
		}
		if (wpConfig.useLeftCoefficients)
		{
			LOG_DEBUGF("C_Lt: %f, %f, %f", wpConfig.C_Lt[0], wpConfig.C_Lt[1], wpConfig.C_Lt[2]);
		}
	}
}

void LoadRainbowPlayerConfigsFromJsonDocument()
{
	LOG_DEBUG("Loading rainbow player configs from JSON document");
	
	// Safety check: Ensure patterns document is valid
	if (patternsDoc.isNull())
	{
		LOG_ERROR("Patterns document is null");
		// Disable rainbow players when no patterns document is available
		rainbowPlayer.setEnabled(false);
		// rainbowPlayer2.setEnabled(false);
		LOG_INFO("Rainbow players disabled - no patterns document available");
		return;
	}
	
	// Safety check: Ensure rainbow player configs array exists
	JsonArray rainbowPlayerConfigsArray = patternsDoc["rainbowPlayerConfigs"];
	if (rainbowPlayerConfigsArray.isNull())
	{
		LOG_ERROR("Rainbow player configs array is null");
		// Disable rainbow players when no rainbow configs are available
		rainbowPlayer.setEnabled(false);
		// rainbowPlayer2.setEnabled(false);
		LOG_INFO("Rainbow players disabled - no rainbow configs available");
		return;
	}

	// Safety check: Validate array size
	int configCount = rainbowPlayerConfigsArray.size();
	LOG_DEBUGF("Found %d rainbow player configs", configCount);
	
	if (configCount <= 0) {
		LOG_WARN("No rainbow player configs found in array");
		rainbowPlayer.setEnabled(false);
		// rainbowPlayer2.setEnabled(false);
		LOG_INFO("Rainbow players disabled - empty config array");
		return;
	}

	// Load configuration for each rainbow player (max 2)
	for (int i = 0; i < configCount && i < 2; i++) // Max 2 rainbow players
	{
		LOG_DEBUGF("Loading rainbow player config %d", i);
		
		// Safety check: Ensure config object is valid
		const JsonObject &config = rainbowPlayerConfigsArray[i];
		if (config.isNull()) {
			LOG_ERRORF("Rainbow player config %d is null", i);
			continue;
		}
		
		// Safety check: Ensure required fields exist
		if (!config.containsKey("name") || !config.containsKey("enabled")) {
			LOG_ERRORF("Rainbow player config %d missing required fields", i);
			continue;
		}
		
		String name = config["name"].as<String>();
		int count = config["count"].as<int>();
		bool enabled = config["enabled"].as<bool>();
		
		LOG_DEBUGF("Rainbow Player %d: %s, count: %d, enabled: %s", 
		           i, name.c_str(), count, enabled ? "true" : "false");
		
		// Apply configuration to the appropriate rainbow player
		if (i == 0) {
			// Configure first rainbow player
			rainbowPlayer.setEnabled(enabled);
			if (enabled) {
				LOG_INFO("Rainbow Player 1 enabled");
			} else {
				LOG_INFO("Rainbow Player 1 disabled");
			}
		} else if (i == 1) {
			// // Configure second rainbow player
			// rainbowPlayer2.setEnabled(enabled);
			// if (enabled) {
			// 	LOG_INFO("Rainbow Player 2 enabled");
			// } else {
			// 	LOG_INFO("Rainbow Player 2 disabled");
			// }
		}
	}
	
	// If we loaded fewer than 2 configs, disable the remaining rainbow players
	if (configCount < 1) {
		rainbowPlayer.setEnabled(false);
		LOG_INFO("Rainbow Player 1 disabled - no config provided");
	}
	// if (configCount < 2) {
	// 	rainbowPlayer2.setEnabled(false);
	// 	LOG_INFO("Rainbow Player 2 disabled - no config provided");
	// }
	
	LOG_DEBUG("Rainbow player configs loading complete");
}

RingPlayer* GetCurrentRingPlayer()
{
	return &unusualRingPlayers[playingRingPlayer];
}

WavePlayer testWavePlayer;
float C_Rt[3] = { 3,2,1 };
float C_Lt[3] = { 3,2,1 };


void Pattern_Setup()
{

	if (LoadPatternsFromJson())
	{
		LOG_DEBUG("Loading wave player configs...");
		LoadWavePlayerConfigsFromJsonDocument();
		LOG_DEBUG("Wave player configs loaded, now loading rainbow player configs...");
		LoadRainbowPlayerConfigsFromJsonDocument();  // Load rainbow player configs
		LOG_DEBUG("All pattern configs loaded successfully");
	}

	auto &testConfig = jsonWavePlayerConfigs[1];
	LOG_DEBUGF("Using config index 1: %s", testConfig.name.c_str());
	LOG_DEBUGF("Config 1 - nTermsRt: %d, nTermsLt: %d", testConfig.nTermsRt, testConfig.nTermsLt);
	LOG_DEBUGF("Config 1 - C_Rt: %f, %f, %f", testConfig.C_Rt[0], testConfig.C_Rt[1], testConfig.C_Rt[2]);
	LOG_DEBUGF("Config 1 - C_Lt: %f, %f, %f", testConfig.C_Lt[0], testConfig.C_Lt[1], testConfig.C_Lt[2]);

	testWavePlayer.init(LightArr[0], testConfig.rows, testConfig.cols, testConfig.onLight, testConfig.offLight);
	testWavePlayer.setWaveData(testConfig.AmpRt, testConfig.wvLenLt, testConfig.wvSpdLt, testConfig.wvLenRt, testConfig.wvSpdRt);
	testWavePlayer.setRightTrigFunc(testConfig.rightTrigFuncIndex);
	testWavePlayer.setLeftTrigFunc(testConfig.leftTrigFuncIndex);

	// Use helper function to set up coefficients properly
	float *rightCoeffs, *leftCoeffs;
	int nTermsRt, nTermsLt;
	SetupWavePlayerCoefficients(testConfig, C_Rt, C_Lt, &rightCoeffs, &leftCoeffs, &nTermsRt, &nTermsLt);

	testWavePlayer.setSeriesCoeffs_Unsafe(rightCoeffs, nTermsRt, leftCoeffs, nTermsLt);
	testWavePlayer.update(0.001f);

	// To avoid flashes when loading user settings
	// UpdateBrightnessInt(0);  // REMOVED: This was causing brightness to be saved as 0 before preferences loaded
	LOG_DEBUG("Initializing pattern data");
	lp2Data[0].init(1, 1, 2);
	lp2Data[0].init(2, 1, 2);
	lp2Data[1].init(3, 1, 10);
	lp2Data[2].init(4, 1, 10);
	lp2Data[3].init(5, 1, 8);
	lp2Data[4].init(6, 1, 10);
	lp2Data[5].init(7, 2, 10);
	lp2Data[6].init(10, 2, 8);
	lp2Data[7].init(11, 2, 8);
	lp2Data[8].init(12, 2, 8);
	lp2Data[9].init(13, 2, 8);
	lp2Data[10].init(14, 2, 10);
	lp2Data[11].init(15, 2, 10);
	lp2Data[12].init(16, 2, 10);
	lp2Data[13].init(31, 2, 10);
	lp2Data[14].init(32, 2, 10);
	lp2Data[15].init(33, 2, 10);
	lp2Data[16].init(34, 2, 8);
	lp2Data[17].init(80, 2, 8);
	lp2Data[16].init(40, 1, 8);
	for (auto &player : firedPatternPlayers)
	{
		player.onLt = Light(255, 255, 255);
		player.offLt = Light(0, 0, 0);
		player.init(LightArr[0], 1, 120, lp2Data[0], 18);
		player.drawOffLt = false;
		player.setToPlaySinglePattern(true);
		player.update();
	}

	rainbowPlayer.setSpeed(5.0f);
	// rainbowPlayer2.setSpeed(5.0f);
	rainbowPlayer.setDirection(true);  // Full array direction
	// rainbowPlayer2.setDirection(true);  // Second half: reverse direction

	pulsePlayer.init(BlendLightArr[0], 32, 32, Light(255, 255, 255), Light(0, 0, 0),	 220, 800.0f, 8.0f, true);

	// Initialize layer system
	layerStack = std::unique_ptr<LayerStack>(new LayerStack(NUM_LEDS));
	layerStack->addLayer<MainLayer>(nullptr, &rainbowPlayer, nullptr);  // Remove second rainbow player
	layerStack->addLayer<PatternLayer>(&pulsePlayer, BlendLightArr);

	// Initialize LightPanels for 2x2 configuration
	// All panels are serpentine (type = 2)
	// Source: 32x32 LightArr (virtual image), Target: leds (physical array)
	LOG_DEBUG("Initializing LightPanels for 2x2 configuration");

	// Set up target addresses for each panel (like your dad's code)
	Light* pTgt = leds;  // Start at beginning of leds array

	// Panel 0: Top-left (0,0) to (15,15) - rows 0-15, cols 0-15
	panels[0].init_Src(FinalLeds, DIMS_PANELS, DIMS_PANELS);
	panels[0].set_SrcArea(16, 16, 0, 0);
	panels[0].pTgt0 = leds;  // LEDs 0-255
	panels[0].type = 2;  // Serpentine
	panels[0].rotIdx = 0;  // No rotation initially
	// pTgt += 256;  // Move to next panel section

	// Panel 1: Top-right (0,16) to (15,31) - rows 0-15, cols 16-31
	panels[1].init_Src(FinalLeds, DIMS_PANELS, DIMS_PANELS);
	panels[1].set_SrcArea(16, 16, 0, 16);
	panels[1].pTgt0 = leds + 256;  // LEDs 256-511
	panels[1].type = 2;  // Serpentine
	panels[1].rotIdx = 0;  // No rotation initially
	// pTgt += 256;  // Move to next panel section

	// Panel 2: Bottom-left (16,0) to (31,15) - rows 16-31, cols 0-15
	panels[2].init_Src(FinalLeds, DIMS_PANELS, DIMS_PANELS);
	panels[2].set_SrcArea(16, 16, 16, 0);
	panels[2].pTgt0 = leds + 512;  // LEDs 512-767
	panels[2].type = 2;  // Serpentine
	panels[2].rotIdx = 0;  // No rotation initially
	// pTgt += 256;  // Move to next panel section

	// Panel 3: Bottom-right (16,16) to (31,31) - rows 16-31, cols 16-31
	panels[3].init_Src(FinalLeds, DIMS_PANELS, DIMS_PANELS);
	panels[3].set_SrcArea(16, 16, 16, 16);
	panels[3].pTgt0 = leds + 768;  // LEDs 768-1023
	panels[3].type = 2;  // Serpentine
	panels[3].rotIdx = 2;  // Rotate 180 degrees

	// lavenderRingPlayer.initToGrid(FinalLeds, 32, 32);
	// lavenderRingPlayer.setRingCenter(2.5f, 26.3f);
	// lavenderRingPlayer.hiLt = Light(125, 0, 60);
	// lavenderRingPlayer.loLt = Light(0, 0, 0);
	// lavenderRingPlayer.ringSpeed = 65.0f;
	// lavenderRingPlayer.ringWidth = 0.7f;
	// // Fades out starting at 6.f, for 10.f rows
	// lavenderRingPlayer.fadeRadius = 1.6f;
	// lavenderRingPlayer.fadeWidth = 2.5f;
	// lavenderRingPlayer.Amp = 1.f;
	// lavenderRingPlayer.onePulse = false;
	// lavenderRingPlayer.direction = -1;
	// lavenderRingPlayer.Start();

	unusualRingPlayers[0].initToGrid(FinalLeds, DIMS_PANELS, DIMS_PANELS);
	unusualRingPlayers[0].setRingCenter(15.5f, 15.5f);
	unusualRingPlayers[0].hiLt = Light(125, 0, 255);
	unusualRingPlayers[0].loLt = Light(0, 0, 0);
	// unusualRingPlayers[0].ringSpeed = 61.1f;
	// unusualRingPlayers[0].ringWidth = 0.5f;
	unusualRingPlayers[0].ringSpeed = 17.1f;
	unusualRingPlayers[0].ringWidth = 0.22f;
	unusualRingPlayers[0].fadeRadius = 3.0f;
	unusualRingPlayers[0].fadeWidth = 6.0f;
	unusualRingPlayers[0].Amp = 1.f;
	unusualRingPlayers[0].onePulse = false;
	unusualRingPlayers[0].Start();
	unusualRingPlayers[0].direction = 1;


	unusualRingPlayers[1].initToGrid(FinalLeds, DIMS_PANELS, DIMS_PANELS);
	unusualRingPlayers[1].setRingCenter(15.5f, 15.5f);
	unusualRingPlayers[1].hiLt = Light(0, 64, 255);
	unusualRingPlayers[1].loLt = Light(0, 0, 0);
	unusualRingPlayers[1].ringSpeed = 70.f;
	unusualRingPlayers[1].ringWidth = 0.46f;
	unusualRingPlayers[1].fadeRadius = 3.5f;
	unusualRingPlayers[1].fadeWidth = 5.0f;
	unusualRingPlayers[1].Amp = 1.f;
	unusualRingPlayers[1].onePulse = false;
	unusualRingPlayers[1].Start();
	unusualRingPlayers[1].direction = 1;

	unusualRingPlayers[2].initToGrid(FinalLeds, DIMS_PANELS, DIMS_PANELS);
	unusualRingPlayers[2].setRingCenter(15.5f, 15.5f);
	unusualRingPlayers[2].hiLt = Light(32, 255, 0);
	unusualRingPlayers[2].loLt = Light(0, 0, 0);
	unusualRingPlayers[2].ringSpeed = 9.61f;
	unusualRingPlayers[2].ringWidth = 0.355f;
	unusualRingPlayers[2].fadeRadius = 3.8f;
	unusualRingPlayers[2].fadeWidth = 5.f;
	unusualRingPlayers[2].Amp = 1.f;
	unusualRingPlayers[2].onePulse = false;
	unusualRingPlayers[2].Start();
	unusualRingPlayers[2].direction = -1;

	unusualRingPlayers[3].initToGrid(FinalLeds, DIMS_PANELS, DIMS_PANELS);
	unusualRingPlayers[3].setRingCenter(15.5f, 15.5f);
	unusualRingPlayers[3].hiLt = Light(0, 255, 255);
	unusualRingPlayers[3].loLt = Light(0, 32, 32);
	unusualRingPlayers[3].ringSpeed = 10.3f;
	unusualRingPlayers[3].ringWidth = 2.5f;
	unusualRingPlayers[3].fadeRadius = 4.0f;
	unusualRingPlayers[3].fadeWidth = 5.0f;
	unusualRingPlayers[3].Amp = 0.5f;
	unusualRingPlayers[3].onePulse = false;
	unusualRingPlayers[3].Start();
	unusualRingPlayers[3].direction = -1;

	LOG_DEBUG("LightPanels initialized successfully");

	// Set up button handlers?
	// if (g_hardwareInputTask) {
	// 	g_hardwareInputTask->getCallbackRegistry().registerCallback("touchButton1", InputEventType::BUTTON_PRESS, [](const InputEvent &event)
	// 		{
	// 			LOG_INFO("Touch button 1 pressed!");
	// 			MoveToNextRingPlayer();
	// 		});
	// 	g_hardwareInputTask->getCallbackRegistry().registerCallback("touchButton2", InputEventType::BUTTON_PRESS, [](const InputEvent &event)
	// 		{
	// 			LOG_INFO("Touch button 2 pressed!");
	// 		});
	// 	g_hardwareInputTask->getCallbackRegistry().registerCallback("touchButton3", InputEventType::BUTTON_PRESS, [](const InputEvent &event)
	// 		{
	// 			LOG_INFO("Touch button 3 pressed!");
	// 		});
	// 	g_hardwareInputTask->getCallbackRegistry().registerDeviceCallback("pot1", [](const InputEvent &event) {
	// 		LOG_INFOF("Pot 1 changed: %d", event.mappedValue);
	// 		auto ringPlayer = GetCurrentRingPlayer();
	// 		// Map 0-255 to 0-1
	// 		// ringPlayer->ringWidth = event.mappedValue / 255.0f;
	// 		// ringPlayer->ringWidth = event.mappedValue / 100.0f;
	// 		// ringPlayer->ringSpeed = event.mappedValue;
	// 	});
	// 	g_hardwareInputTask->getCallbackRegistry().registerDeviceCallback("pot2", [](const InputEvent &event) {
	// 		LOG_INFOF("Pot 2 changed: %d", event.mappedValue);
	// 		auto ringPlayer = GetCurrentRingPlayer();
	// 		// ringPlayer->ringSpeed = event.mappedValue;
	// 		// ringPlayer->ringWidth = event.mappedValue / 255.0f;
	// 		// ringPlayer->ringSpeed = event.mappedValue;
	// 		// UpdateBrightnessInt(event.mappedValue);
	// 	});
	// }
}

void Pattern_Loop()
{
	UpdatePattern();
	UpdateBrightnessPulse();
}

// --- End Pattern Logic Isolation ---

// Helper function to properly set up wave player coefficients
void SetupWavePlayerCoefficients(const WavePlayerConfig &config, float *C_Rt, float *C_Lt,
	float **rightCoeffs, float **leftCoeffs,
	int *nTermsRt, int *nTermsLt)
{
	// Copy right coefficients
	if (config.useRightCoefficients && config.nTermsRt > 0)
	{
		for (int i = 0; i < 3; ++i)
		{
			C_Rt[i] = config.C_Rt[i];
		}
		*rightCoeffs = C_Rt;
		*nTermsRt = config.nTermsRt;
	}
	else
	{
		*rightCoeffs = nullptr;
		*nTermsRt = 0;
	}

	// Handle left coefficients
	if (config.useLeftCoefficients && config.nTermsLt > 0)
	{
		for (int i = 0; i < 3; ++i)
		{
			C_Lt[i] = config.C_Lt[i];
		}
		*leftCoeffs = C_Lt;
		*nTermsLt = config.nTermsLt;
	}
	else if (config.wvLenLt > 0 && config.wvSpdLt > 0)
	{
		// Use basic trig function for left wave (no coefficients)
		*leftCoeffs = nullptr;
		*nTermsLt = 1;
	}
	else
	{
		// No left wave
		*leftCoeffs = nullptr;
		*nTermsLt = 0;
	}

	LOG_DEBUGF("Setup coefficients - right: %s, left: %s",
		*rightCoeffs ? "coefficients" : "basic trig",
		*leftCoeffs ? "coefficients" : (*nTermsLt > 0 ? "basic trig" : "none"));
}


void SwitchWavePlayerIndex(int index)
{
	// auto &config = wavePlayerConfigs[index];
	auto &config = jsonWavePlayerConfigs[index];
	LOG_DEBUGF("Switching to config %d: %s", index, config.name.c_str());
	LOG_DEBUGF("Config %d - nTermsRt: %d, nTermsLt: %d", index, config.nTermsRt, config.nTermsLt);
	LOG_DEBUGF("Config %d - C_Rt: %f, %f, %f", index, config.C_Rt[0], config.C_Rt[1], config.C_Rt[2]);
	LOG_DEBUGF("Config %d - C_Lt: %f, %f, %f", index, config.C_Lt[0], config.C_Lt[1], config.C_Lt[2]);

	testWavePlayer.nTermsLt = config.nTermsLt;
	testWavePlayer.nTermsRt = config.nTermsRt;

	testWavePlayer.init(LightArr[0], config.rows, config.cols, config.onLight, config.offLight);
	testWavePlayer.setWaveData(config.AmpRt, config.wvLenLt, config.wvSpdLt, config.wvLenRt, config.wvSpdRt);
	testWavePlayer.setRightTrigFunc(config.rightTrigFuncIndex);
	testWavePlayer.setLeftTrigFunc(config.leftTrigFuncIndex);

	// Use helper function to set up coefficients properly
	float *rightCoeffs, *leftCoeffs;
	int nTermsRt, nTermsLt;
	SetupWavePlayerCoefficients(config, C_Rt, C_Lt, &rightCoeffs, &leftCoeffs, &nTermsRt, &nTermsLt);

	testWavePlayer.setSeriesCoeffs_Unsafe(rightCoeffs, nTermsRt, leftCoeffs, nTermsLt);
}


void GoToNextPattern()
{
	currentWavePlayerIndex++;
	if (currentWavePlayerIndex >= numWavePlayerConfigs)
	{
		currentWavePlayerIndex = 0;
	}
	SwitchWavePlayerIndex(currentWavePlayerIndex);
	sharedCurrentIndexState = 0;
	Serial.println("GoToNextPattern" + String(currentWavePlayerIndex));
	UpdateAllCharacteristicsForCurrentPattern();
}

void GoToPattern(int patternIndex)
{
	currentWavePlayerIndex = patternIndex;
	sharedCurrentIndexState = 0;
	Serial.println("GoToPattern" + String(currentWavePlayerIndex));
	SwitchWavePlayerIndex(currentWavePlayerIndex);
	UpdateAllCharacteristicsForCurrentPattern();
}

void UpdateAlertWavePlayer(float dtSeconds)
{
	if (alertWavePlayerActive) {
		alertWavePlayer.update(dtSeconds);
	}
}

void BlendWavePlayers()
{
	if (!alertWavePlayerActive) {
		return;  // No blending needed if no alert
	}
	
	// Create a pulsing fade effect for the alert overlay
	static float fadeTime = 0.0f;
	fadeTime += 0.02f;  // Adjust speed of fade cycle
	if (fadeTime > 2.0f * PI) {
		fadeTime -= 2.0f * PI;
	}
	
	// Create a smooth sine wave fade (0.0 to 1.0)
	float fadeIntensity = (sin(fadeTime) + 1.0f) * 0.5f;
	
	// Blend the alert pattern with the main pattern
	for (int i = 0; i < NUM_LEDS; ++i) {
		// Get the alert color for this LED from the separate alert buffer
		Light alertColor = AlertLightArr[i];
		
		// Blend: mainColor * (1 - fadeIntensity) + alertColor * fadeIntensity
		LightArr[i].r = (uint8_t)((float)LightArr[i].r * (1.0f - fadeIntensity) + (float)alertColor.r * fadeIntensity);
		LightArr[i].g = (uint8_t)((float)LightArr[i].g * (1.0f - fadeIntensity) + (float)alertColor.g * fadeIntensity);
		LightArr[i].b = (uint8_t)((float)LightArr[i].b * (1.0f - fadeIntensity) + (float)alertColor.b * fadeIntensity);
	}
}

// Thermal brightness curve mapping - uses same exponential curve as potentiometer
float getThermalBrightnessCurve(float currentBrightnessNormalized)
{
    // Use the same exponential curve as potentiometer: (exp(value) - 1) / (exp(1) - 1)
    const auto numerator = exp(currentBrightnessNormalized) - 1.0f;
    const auto denominator = exp(1.0f) - 1.0f;
    return numerator / denominator;
}

void UpdatePattern()
{
	static unsigned long lastUpdateTime = micros();
	const auto now = micros();
	const auto dt = now - lastUpdateTime;
	// Convert dt to seconds (micros() returns microseconds)
	// To match old millis() behavior: 16ms * 0.0001f = 0.0016s
	// So: 16000μs * 0.0000001f = 0.0016s
	float dtSeconds = dt * 0.0000001f;
	
	// Handle micros() overflow (happens every ~70 minutes)
	if (dt < 0) {
		dtSeconds = 0.0016f;  // Default to 0.0016 seconds if overflow detected
	}
	
	lastUpdateTime = now;


	// Update alert wave player if active
	UpdateAlertWavePlayer(dtSeconds);
	
	// Blend alert with main pattern if alert is active
	BlendWavePlayers();
	
	// FORCE brightness reduction for thermal management - this overrides everything else
	if (alertWavePlayerActive) {
		extern DeviceState deviceState;
		// Use exponential curve mapping for thermal brightness reduction
		float currentBrightnessNormalized = deviceState.brightness / 255.0f;  // Convert to 0.0-1.0
		float thermalCurveValue = getThermalBrightnessCurve(currentBrightnessNormalized);
		int reducedBrightness = max(25, (int)(thermalCurveValue * 255.0f * 0.3f));  // Apply 30% of curve value
		
		if (deviceState.brightness > reducedBrightness) {
			// deviceState.brightness = reducedBrightness;
			FastLED.setBrightness(reducedBrightness);
			// Brightness is now managed by BrightnessController
			// BLEManager::getInstance()->updateBrightness();
		}
	}

	// Use layer system to update and render
	if (layerStack) {
		// Get current speed from SpeedController in real-time
		SpeedController* speedController = SpeedController::getInstance();
		float currentSpeed = speedController ? speedController->getSpeed() : speedMultiplier;
		layerStack->update(dtSeconds * currentSpeed);
		layerStack->render(FinalLeds);  // Render to leds first
	}


	// Temporarily disabled ring players to test baseline performance
	// unusualRingPlayers[2].update(0.033f);
	// 	unusualRingPlayers[3].update(0.033f);
	// lavenderRingPlayer.update(0.033f);
	// unusualRingPlayers[1].update(0.033f);
	// unusualRingPlayers[playingRingPlayer].update(dtSeconds);

	// LightPanels read from leds and write back to leds with transformations
	for (int i = 0; i < 4; i++) {
		panels[i].update();
	}
}

void UpdateCurrentPatternColors(Light newHighLt, Light newLowLt)
{
	WavePlayer *currentWavePlayer = GetCurrentWavePlayer();
	currentWavePlayer->hiLt = newHighLt;
	currentWavePlayer->loLt = newLowLt;
	currentWavePlayer->init(LightArr[0], currentWavePlayer->rows, currentWavePlayer->cols, newHighLt, newLowLt);
	UpdateAllCharacteristicsForCurrentPattern();
}

WavePlayer *GetCurrentWavePlayer()
{
	return &testWavePlayer;
}

std::pair<Light, Light> GetCurrentPatternColors()
{
	return std::make_pair(testWavePlayer.hiLt, testWavePlayer.loLt);
}

void FirePatternFromBLE(int idx, Light on, Light off)
{
	const auto numPatterns = lp2Data.size();
	if (idx < 0 || idx >= numPatterns)
	{
		Serial.println("Invalid pattern index - must be 0-" + String(numPatterns - 1));
		return;
	}
	Serial.println("Trying to fire pattern " + String(idx));
	const auto playerIdx = findAvailablePatternPlayer();
	if (playerIdx == -1)
	{
		Serial.println("No available pattern player found");
		return;
	}
	Serial.println("Firing pattern " + String(idx) + " on player " + String(playerIdx));
	firedPatternPlayers[playerIdx].setToPlaySinglePattern(true);
	firedPatternPlayers[playerIdx].drawOffLt = false;
	firedPatternPlayers[playerIdx].onLt = on;
	firedPatternPlayers[playerIdx].offLt = off;
	firedPatternPlayers[playerIdx].firePattern(idx);
}

unsigned int findAvailablePatternPlayer()
{
	for (unsigned int i = 0; i < firedPatternPlayers.size(); ++i)
	{
		if (!firedPatternPlayers[i].isPlayingSinglePattern())
		{
			return i;
		}
	}
	return -1;
}

// ===== Functions moved from main.cpp =====

void UpdateAllCharacteristicsForCurrentPattern()
{
	// Update pattern index characteristic
	BLEManager::getInstance()->getPatternIndexCharacteristic().writeValue(String(currentWavePlayerIndex));

	// Update color characteristics based on current pattern
	const auto currentColors = GetCurrentPatternColors();
	String highColorStr = String(currentColors.first.r) + "," + String(currentColors.first.g) + "," + String(currentColors.first.b);
	String lowColorStr = String(currentColors.second.r) + "," + String(currentColors.second.g) + "," + String(currentColors.second.b);

	BLEManager::getInstance()->getHighColorCharacteristic().writeValue(highColorStr);
	BLEManager::getInstance()->getLowColorCharacteristic().writeValue(lowColorStr);

	// Brightness is now managed by BrightnessController
	// BLEManager::getInstance()->updateBrightness();

	// Update series coefficients if applicable
		// Get the appropriate wave player for the current pattern
	WavePlayer *currentWavePlayer = GetCurrentWavePlayer();

	if (currentWavePlayer)
	{
		// Format series coefficients as strings
		String leftCoeffsStr = "0.0,0.0,0.0";
		String rightCoeffsStr = "0.0,0.0,0.0";

		if (currentWavePlayer->C_Lt && currentWavePlayer->nTermsLt > 0)
		{
			leftCoeffsStr = String(currentWavePlayer->C_Lt[0], 2) + "," +
				String(currentWavePlayer->C_Lt[1], 2) + "," +
				String(currentWavePlayer->C_Lt[2], 2);
		}

		if (currentWavePlayer->C_Rt && currentWavePlayer->nTermsRt > 0)
		{
			rightCoeffsStr = String(currentWavePlayer->C_Rt[0], 2) + "," +
				String(currentWavePlayer->C_Rt[1], 2) + "," +
				String(currentWavePlayer->C_Rt[2], 2);
		}

		BLEManager::getInstance()->getLeftSeriesCoefficientsCharacteristic().writeValue(leftCoeffsStr);
		BLEManager::getInstance()->getRightSeriesCoefficientsCharacteristic().writeValue(rightCoeffsStr);
	}
}

void UpdateColorFromCharacteristic(BLEStringCharacteristic &characteristic, Light &color, bool isHighColor)
{
	const auto value = characteristic.value();
	Serial.println("Color characteristic written" + String(value));
	// Cast from int,int,int to strings, but integers might not all be the same length
	// So parse from comma to comma

	// Split the string into r,g,b
	String r = value.substring(0, value.indexOf(','));
	String g = value.substring(value.indexOf(',') + 1, value.lastIndexOf(','));
	String b = value.substring(value.lastIndexOf(',') + 1);


	Serial.println("R: " + String(r));
	Serial.println("G: " + String(g));
	Serial.println("B: " + String(b));

	int rInt = r.toInt();
	int gInt = g.toInt();
	int bInt = b.toInt();
	Serial.println("Setting color to: " + String(rInt) + "," + String(gInt) + "," + String(bInt));
	Light newColor = Light(rInt, gInt, bInt);
	const auto currentPatternColors = GetCurrentPatternColors();
	if (isHighColor)
	{
		UpdateCurrentPatternColors(newColor, currentPatternColors.second);
	}
	else
	{
		UpdateCurrentPatternColors(currentPatternColors.first, newColor);
	}
}

void UpdateSeriesCoefficientsFromCharacteristic(BLEStringCharacteristic &characteristic, WavePlayer &wp)
{
	const auto value = characteristic.value();
	Serial.println("Series coefficients characteristic written" + String(value));

	float leftCoeffsArray[3];
	float rightCoeffsArray[3];

	if (wp.nTermsLt > 0)
	{
		// Parse from string like 0.95,0.3,1.2
		String coeffs = value.substring(0, value.indexOf(','));
		String coeffs2 = value.substring(value.indexOf(',') + 1, value.lastIndexOf(','));
		String coeffs3 = value.substring(value.lastIndexOf(',') + 1);
		leftCoeffsArray[0] = coeffs.toFloat();
		leftCoeffsArray[1] = coeffs2.toFloat();
		leftCoeffsArray[2] = coeffs3.toFloat();
	}

	if (wp.nTermsRt > 0)
	{
		// Parse from string like 0.95,0.3,1.2
		String coeffs = value.substring(0, value.indexOf(','));
		String coeffs2 = value.substring(value.indexOf(',') + 1, value.lastIndexOf(','));
		String coeffs3 = value.substring(value.lastIndexOf(',') + 1);
		rightCoeffsArray[0] = coeffs.toFloat();
		rightCoeffsArray[1] = coeffs2.toFloat();
		rightCoeffsArray[2] = coeffs3.toFloat();
	}

	wp.setSeriesCoeffs_Unsafe(leftCoeffsArray, 3, rightCoeffsArray, 3);

	// Update characteristics to reflect the new series coefficients
	UpdateAllCharacteristicsForCurrentPattern();
}

Light ParseColor(const String &colorStr)
{
	// Parse from string like (r,g,b)
	String r = colorStr.substring(0, colorStr.indexOf(','));
	String g = colorStr.substring(colorStr.indexOf(',') + 1, colorStr.lastIndexOf(','));
	String b = colorStr.substring(colorStr.lastIndexOf(',') + 1);
	int rInt = r.toInt();
	int gInt = g.toInt();
	int bInt = b.toInt();
	return Light(rInt, gInt, bInt);
}

// Parse a fire_pattern command like fire_pattern:<idx>-(args-args-args,etc)
// but here we only need to get the idx and the rest is handled by
// the callee (firePatternFromBLE handles dealing with the args and turning it into a
// param if needed, or on/off colors, etc)
void ParseFirePatternCommand(const String &command)
{
	// Find the dash separator
	int dashIndex = command.indexOf('-');
	if (dashIndex == -1)
	{
		Serial.println("Invalid fire_pattern format - expected idx-on-off");
		return;
	}

	// Extract the index
	String idxStr = command.substring(0, dashIndex);
	int idx = idxStr.toInt();

	// Extract the on and off colors
	String onStr = command.substring(dashIndex + 1, command.indexOf('-', dashIndex + 1));
	String offStr = command.substring(command.indexOf('-', dashIndex + 1) + 1);

	// Parse the on and off colors
	Light on = ParseColor(onStr);
	Light off = ParseColor(offStr);
	FirePatternFromBLE(idx, on, off);
}

void ParseAndExecuteCommand(const String &command)
{
	Serial.println("Parsing command: " + command);

	// Find the colon separator
	int colonIndex = command.indexOf(':');
	if (colonIndex == -1)
	{
		Serial.println("Invalid command format - missing colon");
		return;
	}

	// Extract command and arguments
	String cmd = command.substring(0, colonIndex);
	String args = command.substring(colonIndex + 1);

	cmd.trim();
	args.trim();

	Serial.println("Command: " + cmd);
	Serial.println("Args: " + args);

	if (cmd == "pulse_brightness")
	{
		// Parse arguments: target_brightness,duration_ms
		int commaIndex = args.indexOf(',');
		if (commaIndex == -1)
		{
			Serial.println("Invalid pulse_brightness format - expected target,duration");
			return;
		}

		String targetStr = args.substring(0, commaIndex);
		String durationStr = args.substring(commaIndex + 1);

		int targetBrightness = targetStr.toInt();
		unsigned long duration = durationStr.toInt();

		if (targetBrightness < 0 || targetBrightness > 255)
		{
			Serial.println("Invalid target brightness - must be 0-255");
			return;
		}

		if (duration <= 0)
		{
			Serial.println("Invalid duration - must be positive");
			return;
		}

		StartBrightnessPulse(targetBrightness, duration);
		Serial.println("Started brightness pulse to " + String(targetBrightness) + " over " + String(duration) + "ms");
	}
	else if (cmd == "fade_brightness")
	{
		// Parse arguments: target_brightness,duration_ms
		int commaIndex = args.indexOf(',');
		if (commaIndex == -1)
		{
			Serial.println("Invalid fade_brightness format - expected target,duration");
			return;
		}

		String targetStr = args.substring(0, commaIndex);
		String durationStr = args.substring(commaIndex + 1);

		int targetBrightness = targetStr.toInt();
		unsigned long duration = durationStr.toInt();

		if (targetBrightness < 0 || targetBrightness > 255)
		{
			Serial.println("Invalid target brightness - must be 0-255");
			return;
		}

		if (duration <= 0)
		{
			Serial.println("Invalid duration - must be positive");
			return;
		}

		StartBrightnessFade(targetBrightness, duration);
		Serial.println("Started brightness fade to " + String(targetBrightness) + " over " + String(duration) + "ms");
	}
	else if (cmd == "fire_pattern")
	{
		// Command will look like fire_pattern:<idx>-<string-args>
		ParseFirePatternCommand(args);
	}
	else if (cmd == "ping")
	{
		// Echo back the timestamp
		Serial.println("Ping -- pong");
		BLEManager::getInstance()->getCommandCharacteristic().writeValue("pong:" + args);
	}
	else
	{
		Serial.println("Unknown command: " + cmd);
	}
}

void StartBrightnessPulse(int targetBrightness, unsigned long duration)
{
	BrightnessController* brightnessController = BrightnessController::getInstance();
	if (brightnessController) {
		brightnessController->startPulse(targetBrightness, duration);
	} else {
		// Fallback to old implementation
		extern DeviceState deviceState;
		previousBrightness = deviceState.brightness;
		pulseTargetBrightness = targetBrightness;
		pulseDuration = duration;
		pulseStartTime = millis();
		isPulsing = true;
		isFadeMode = false;
		Serial.println("Starting brightness pulse from " + String(previousBrightness) + " to " + String(targetBrightness));
	}
}

void StartBrightnessFade(int targetBrightness, unsigned long duration)
{
	BrightnessController* brightnessController = BrightnessController::getInstance();
	if (brightnessController) {
		brightnessController->startFade(targetBrightness, duration);
	} else {
		// Fallback to old implementation
		extern DeviceState deviceState;
		previousBrightness = deviceState.brightness;
		pulseTargetBrightness = targetBrightness;
		pulseDuration = duration;
		pulseStartTime = millis();
		isPulsing = true;
		isFadeMode = true;
		Serial.println("Starting brightness fade from " + String(previousBrightness) + " to " + String(targetBrightness));
	}
}

void UpdateBrightnessPulse()
{
	BrightnessController* brightnessController = BrightnessController::getInstance();
	if (brightnessController) {
		brightnessController->update();
	} else {
		// Fallback to old implementation
		if (!isPulsing) {
			return;
		}

		// Don't override brightness if there's an active temperature alert
		if (alertWavePlayerActive) {
			// Stop any active pulse/fade when thermal management takes over
			isPulsing = false;
			return;
		}

		unsigned long currentTime = millis();
		unsigned long elapsed = currentTime - pulseStartTime;

		if (elapsed >= pulseDuration) {
			// Transition complete - set to final brightness
			extern DeviceState deviceState;

			if (isFadeMode) {
				// For fade, stay at target brightness
				deviceState.brightness = pulseTargetBrightness;
				FastLED.setBrightness(deviceState.brightness);
				BLEManager::getInstance()->updateBrightness();
				Serial.println("Brightness fade complete - now at " + String(pulseTargetBrightness));
			} else {
				// For pulse, return to previous brightness
				deviceState.brightness = previousBrightness;
				FastLED.setBrightness(deviceState.brightness);
				BLEManager::getInstance()->updateBrightness();
				Serial.println("Brightness pulse complete - returned to " + String(previousBrightness));
			}
			isPulsing = false;
			return;
		}

		// Calculate current brightness using smooth interpolation
		float progress = (float) elapsed / (float) pulseDuration;

		float smoothProgress;
		if (isFadeMode) {
			// Linear interpolation for fade (simple and smooth)
			smoothProgress = progress;
		} else {
			// Use smooth sine wave interpolation for natural pulsing effect
			// This creates a full cycle: 0 -> 1 -> 0 over the duration
			smoothProgress = (sin(progress * 2 * PI - PI / 2) + 1) / 2; // Full cycle: 0 to 1 to 0
		}

		extern DeviceState deviceState;
		int currentBrightness = previousBrightness + (int) ((pulseTargetBrightness - previousBrightness) * smoothProgress);

		// Apply the calculated brightness
		deviceState.brightness = currentBrightness;
		FastLED.setBrightness(deviceState.brightness);
		BLEManager::getInstance()->updateBrightness();
	}
}

void UpdateBrightnessInt(int value)
{
	LOG_DEBUG("Updating brightness to " + String(value));
	// Use the BrightnessController instead of managing brightness directly
	BrightnessController* brightnessController = BrightnessController::getInstance();
	if (brightnessController) {
		LOG_DEBUG("Using BrightnessController");
		brightnessController->setBrightness(value);
	} else {
		// Fallback to direct management if controller not available
		LOG_DEBUG("Using direct management");
		extern DeviceState deviceState;
		deviceState.brightness = value;
		FastLED.setBrightness(deviceState.brightness);
		BLEManager::getInstance()->updateBrightness();
	}
}

void UpdateBrightness(float value)
{
	extern DeviceState deviceState;
	deviceState.brightness = value * 255;
	FastLED.setBrightness(deviceState.brightness);
}


void ApplyFromUserPreferences(DeviceState &state, bool skipBrightness)
{
	// Use SpeedController if available, otherwise fall back to direct assignment
	SpeedController* speedController = SpeedController::getInstance();
	if (speedController) {
		Serial.println("Applying from user preferences: speedMultiplier = " + String(state.speedMultiplier));
		speedController->syncWithDeviceState(state);
	} else {
		// Fallback to direct assignment if SpeedController not available
		speedMultiplier = state.speedMultiplier;
		Serial.println("Applying from user preferences: speedMultiplier = " + String(speedMultiplier));
	}
	
	// Skip brightness if explicitly requested OR if there's an active temperature alert
	if (!skipBrightness && !alertWavePlayerActive) {
		Serial.println("Applying from user preferences: brightness = " + String(state.brightness));
		// Use syncWithDeviceState to avoid triggering BLE callback during preference loading
		BrightnessController* brightnessController = BrightnessController::getInstance();
		if (brightnessController) {
			Serial.println("BrightnessController instance found, calling syncWithDeviceState");
			brightnessController->syncWithDeviceState(state);
		} else {
			Serial.println("BrightnessController instance not found, using fallback");
			// Fallback to direct management if controller not available
			extern DeviceState deviceState;
			deviceState.brightness = state.brightness;
			FastLED.setBrightness(deviceState.brightness);
		}
	} else if (alertWavePlayerActive) {
		Serial.println("Skipping brightness from user preferences due to active temperature alert");
	} else {
		Serial.println("Skipping brightness from user preferences (explicitly requested)");
	}
	
	GoToPattern(state.patternIndex);
	Serial.println("Applying from user preferences: patternIndex = " + String(state.patternIndex));
	// UpdateCurrentPatternColors(state.highColor, state.lowColor);
	// Serial.println("Applying from user preferences: highColor = " + String(state.highColor.r) + "," + String(state.highColor.g) + "," + String(state.highColor.b));
	// Serial.println("Applying from user preferences: lowColor = " + String(state.lowColor.r) + "," + String(state.lowColor.g) + "," + String(state.lowColor.b));
}

void SaveUserPreferences(const DeviceState &state)
{
	Serial.println("Saving user preferences");
	
	// Create a copy of the state with current values from controllers
	DeviceState currentState = state;
	
	// Get current speed from SpeedController
	SpeedController* speedController = SpeedController::getInstance();
	if (speedController) {
		currentState.speedMultiplier = speedController->getSpeed();
		Serial.println("Saving current speed from SpeedController: " + String(currentState.speedMultiplier));
	}
	
	// Get current brightness from BrightnessController
	BrightnessController* brightnessController = BrightnessController::getInstance();
	if (brightnessController) {
		currentState.brightness = brightnessController->getBrightness();
		Serial.println("Saving current brightness from BrightnessController: " + String(currentState.brightness));
	}
	
	prefsManager.save(currentState);
}

void SetAlertWavePlayer(String reason)
{
	Serial.println("Setting alert wave player: " + reason);
	if (reason == "high_temperature") {
		// Create a bright red alert pattern that will overlay nicely
		// Use separate buffer so it doesn't overwrite the main pattern
		alertWavePlayer.init(AlertLightArr[0], 16, 16, Light(255, 0, 0), Light(0, 0, 0));
		// Use a different wave pattern than the main one for visual distinction
		alertWavePlayer.setWaveData(0.8f, 30.f, 15.f, 30.f, 15.f);  // Different wavelength and speed
		alertWavePlayer.setRightTrigFunc(1);  // Use cosine instead of sine
		alertWavePlayer.setLeftTrigFunc(1);   // Use cosine for left wave too
		alertWavePlayer.update(0.f);
	}
	alertWavePlayerActive = true;
}

void StopAlertWavePlayer(String reason)
{
	Serial.println("Stopping alert wave player: " + reason);
	alertWavePlayerActive = false;
}


