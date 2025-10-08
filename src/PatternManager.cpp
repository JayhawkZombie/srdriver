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
#include "lights/patterns/ConfigManager.h"
#include "lights/patterns/PlayerManager.h"

// Add externs for all globals and helpers used by pattern logic
unsigned int findAvailablePatternPlayer();
void SetupWavePlayerCoefficients(const WavePlayerConfig &config, float *C_Rt, float *C_Lt, float **rightCoeffs, float **leftCoeffs, int *nTermsRt, int *nTermsLt);

// Global ConfigManager instance
ConfigManager* g_configManager = nullptr;

// Global PlayerManager instance
PlayerManager* g_playerManager = nullptr;

// Pattern-related global definitions
int currentWavePlayerIndex = 0;
int currentPatternIndex = 0;
Light LightArr[NUM_LEDS];
Light BlendLightArr[NUM_LEDS];
Light FinalLeds[NUM_LEDS];

Button pushButton(PUSHBUTTON_PIN);
Button pushButtonSecondary(PUSHBUTTON_PIN_SECONDARY);
bool rainbowPlayerActive = false;
DataPlayer dp;
int sharedCurrentIndexState = 0;
// Global speedMultiplier for backward compatibility with SpeedController
float speedMultiplier = 4.0f;  // Default value, will be updated by SpeedController

WavePlayer alertWavePlayer;
bool alertWavePlayerActive = false;

// Add separate buffer for alert wave player
Light AlertLightArr[NUM_LEDS];

// Forward declarations for BLE manager access
// extern BLEManager bleManager;

// --- Pattern Logic Isolation ---
extern JsonSettings settings;
extern SDCardController *g_sdCardController;


extern HardwareInputTask *g_hardwareInputTask;





RingPlayer* GetCurrentRingPlayer()
{
	return g_playerManager ? g_playerManager->getCurrentRingPlayer() : nullptr;
}

WavePlayer testWavePlayer;
float C_Rt[3] = { 3,2,1 };
float C_Lt[3] = { 3,2,1 };


void Pattern_Setup()
{
	// Create and initialize ConfigManager
	g_configManager = new ConfigManager();
	if (!g_configManager->loadPatterns()) {
		LOG_ERROR("Pattern setup: Failed to load patterns");
		return;
	}

	// Create and initialize PlayerManager
	g_playerManager = new PlayerManager();
	g_playerManager->setup(g_configManager);
	
	LOG_DEBUG("Pattern setup: All players initialized successfully");

}

void Pattern_Loop()
{
	static unsigned long lastUpdateTime = micros();
	const auto now = micros();
	const auto dt = now - lastUpdateTime;
	float dtSeconds = dt * 0.0000001f;
	
	if (dt < 0) {
		dtSeconds = 0.0016f;  // Handle micros() overflow
	}
	lastUpdateTime = now;

	// Update all players through PlayerManager
	if (g_playerManager) {
		g_playerManager->update(dtSeconds);
	}
	
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
	if (g_playerManager) {
		g_playerManager->switchWavePlayer(index);
	}
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

	// All player updates are now handled by PlayerManager in Pattern_Loop()
}

void UpdateCurrentPatternColors(Light newHighLt, Light newLowLt)
{
	if (g_playerManager) {
		g_playerManager->updateWavePlayerColors(newHighLt, newLowLt);
	}
}

WavePlayer *GetCurrentWavePlayer()
{
	return g_playerManager ? g_playerManager->getCurrentWavePlayer() : nullptr;
}

std::pair<Light, Light> GetCurrentPatternColors()
{
	return g_playerManager ? g_playerManager->getCurrentWavePlayerColors() : std::make_pair(Light(0,0,0), Light(0,0,0));
}

void FirePatternFromBLE(int idx, Light on, Light off)
{
	if (g_playerManager) {
		g_playerManager->firePattern(idx, on, off);
	}
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

// ===== Runtime Configuration Functions =====

void EnableRainbowPlayer(bool enabled) {
	if (g_playerManager) {
		g_playerManager->setRainbowPlayerEnabled(enabled);
	}
}

void SetRainbowPlayerSpeed(float speed) {
	if (g_playerManager) {
		g_playerManager->setRainbowPlayerSpeed(speed);
	}
}

void EnablePulsePlayer(bool enabled) {
	if (g_playerManager) {
		g_playerManager->setPulsePlayerEnabled(enabled);
	}
}

void SetPulsePlayerParameters(float frequency, float amplitude, float phase, bool continuous) {
	if (g_playerManager) {
		g_playerManager->setPulsePlayerParameters(frequency, amplitude, phase, continuous);
	}
}

void EnableRingPlayer(int index, bool enabled) {
	if (g_playerManager) {
		g_playerManager->setRingPlayerEnabled(index, enabled);
	}
}

void SetRingPlayerParameters(int index, float speed, float width, Light highColor, Light lowColor) {
	if (g_playerManager) {
		g_playerManager->setRingPlayerParameters(index, speed, width, highColor, lowColor);
	}
}

void MoveToNextRingPlayer() {
	if (g_playerManager) {
		g_playerManager->moveToNextRingPlayer();
	}
}


