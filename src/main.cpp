#define FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED 0

#include <FastLED.h>
#include <stdint.h>
#include "Behaviors/Noise.hpp"
#include "Utils.hpp"
#include "Globals.h"
#include "Behaviors/Pulser.hpp"
#include "Behaviors/ReversePulser.hpp"
#include "Behaviors/Ring.hpp"
#include "Behaviors/ColumnsRows.hpp"
#include "Behaviors/Diagonals.hpp"
#include "LightPlayer2.h"
#include "DataPlayer.h"
#include <Adafruit_NeoPixel.h>
#include <Utils.hpp>
#include "hal/Button.hpp"
#include "hal/Potentiometer.hpp"
#include "die.hpp"
#include "WavePlayer.h"
#include <ArduinoBLE.h>

#include "GlobalState.h"
#include "DeviceState.h"
#include "BLEManager.h"
#include "PatternManager.h"
DeviceState deviceState;
BLEManager bleManager(deviceState, GoToPattern);

#if FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED
Rgbw rgbw = Rgbw(
	kRGBWDefaultColorTemp,
	kRGBWExactColors,      // Mode
	W3                     // W-placement
);

typedef WS2812<LED_PIN, RGB> ControllerT;  // RGB mode must be RGB, no re-ordering allowed.
static RGBWEmulatedController<ControllerT, GRB> rgbwEmu(rgbw);  // ordering goes here.
#endif

// LightPlayer2 uses 
Light onLt(200, 0, 60);// these
Light offLt(60, 0, 200);// Lights

// Button and Potentiometer instances
Potentiometer brightnessPot(POTENTIOMETER_PIN_BRIGHTNESS);
Potentiometer speedPot(POTENTIOMETER_PIN_SPEED);
Potentiometer extraPot(POTENTIOMETER_PIN_EXTRA);

CRGB leds[NUM_LEDS];

// LightPlayer2 LtPlay2; // Declare the LightPlayer2 instance

// storage for a 3 step pattern #100
uint8_t stateData[24];// enough for 24*8 = 192 = 3*64 state assignments

WavePlayer largeWavePlayer;
DataPlayer dataPlayer;

void EnterSettingsMode();
void MoveToNextSetting();
void ExitSettingsMode();
void UpdateLEDsForSettings(int potentiometerValue);
void ParseAndExecuteCommand(const String &command);
void StartBrightnessPulse(int targetBrightness, unsigned long duration);
void UpdateBrightnessPulse();

// Heartbeat timing
unsigned long lastHeartbeatSent = 0;
const unsigned long HEARTBEAT_INTERVAL_MS = 5000;

void wait_for_serial_connection()
{
	uint32_t timeout_end = millis() + 2000;
	Serial.begin(9600);
	while (!Serial && timeout_end > millis()) {}  //wait until the connection to the PC is established
}

void OnSettingChanged(DeviceState &state)
{
	FastLED.setBrightness(state.brightness);
	// Optionally: save preferences, update UI, etc.
}

void setup()
{
	wait_for_serial_connection();

	if (!BLE.begin())
	{
		Serial.println("Failed to initialize BLE");
		while (1) {};
	}

	BLE.setLocalName("SRDriver");
	BLE.setDeviceName("SRDriver");
	// Serial.print("BLE local name: ");
	// Serial.println(BLE.localName());

	BLE.advertise();
	Serial.println("BLE initialized");

	// Used for RGB (NOT RGBW) LED strip
#if FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED
	FastLED.addLeds(&rgbwEmu, leds, NUM_LEDS);
#else
	FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
#endif
	FastLED.setBrightness(BRIGHTNESS);
	// Control power usage if computer is complaining/LEDs are misbehaving
	// FastLED.setMaxPowerInVoltsAndMilliamps(5, NUM_LEDS * 20);
	Serial.println("Setup");

	Pattern_Setup();

	// Add heartbeat characteristic
	bleManager.getHeartbeatCharacteristic().writeValue(millis());

	Serial.println("Setup complete");
	pinMode(PUSHBUTTON_PIN, INPUT_PULLUP);
	pinMode(PUSHBUTTON_PIN_SECONDARY, INPUT_PULLUP);

	bleManager.begin();
	bleManager.setOnSettingChanged(OnSettingChanged);
}



unsigned long maxDelay = 505;
unsigned long minDelay = 50;
fract8 curr = 0;
unsigned long getNextDelay(unsigned long i)
{
	return InterpolateCubicFloat(minDelay, maxDelay, i / 64.f);
}

void DrawError(const CRGB &color)
{
	for (int i = 0; i < LEDS_MATRIX_X; i += 2)
	{
		leds[i] = color;
	}
}

unsigned long lastUpdateMs = 0;
int sharedCurrentIndexState = 0;
unsigned long last_ms = 0;
float speedMultiplier = 8.0f;

void UpdateAllCharacteristicsForCurrentPattern()
{
	// Update pattern index characteristic
	bleManager.getPatternIndexCharacteristic().writeValue(String(currentWavePlayerIndex));

	// Update color characteristics based on current pattern
	const auto currentColors = GetCurrentPatternColors();
	String highColorStr = String(currentColors.first.r) + "," + String(currentColors.first.g) + "," + String(currentColors.first.b);
	String lowColorStr = String(currentColors.second.r) + "," + String(currentColors.second.g) + "," + String(currentColors.second.b);

	bleManager.getHighColorCharacteristic().writeValue(highColorStr);
	bleManager.getLowColorCharacteristic().writeValue(lowColorStr);

	// Update brightness characteristic
	bleManager.updateBrightness();

	// Update series coefficients if applicable
	const auto currentPattern = patternOrder[currentPatternIndex % patternOrder.size()];
	if (currentPattern == PatternType::WAVE_PLAYER_PATTERN)
	{
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

			bleManager.getLeftSeriesCoefficientsCharacteristic().writeValue(leftCoeffsStr);
			bleManager.getRightSeriesCoefficientsCharacteristic().writeValue(rightCoeffsStr);
		}
	}
}

void UpdateBrightnessInt(int value)
{
	deviceState.brightness = value;
	FastLED.setBrightness(deviceState.brightness);
	bleManager.updateBrightness();
}

void UpdateBrightness(float value)
{
	deviceState.brightness = value * 255;
	FastLED.setBrightness(deviceState.brightness);
}

void CheckPotentiometers()
{
	// Always check brightness potentiometer regardless of potensControlColor
	// Call getValue() to update the change detection state
	brightnessPot.getValue();

	if (brightnessPot.hasChanged())
	{
		Serial.println("Brightness potentiometer has changed");
		float brightness = brightnessPot.getCurveMappedValue();
		UpdateBrightness(brightness);
		bleManager.updateBrightness();
		brightnessPot.resetChanged();
	}

	int speed = speedPot.getMappedValue(0, 255);
	int extra = extraPot.getMappedValue(0, 255);
	speedMultiplier = speed / 255.f * 20.f;
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

void HandleBLE()
{
	static bool connected = false;
	BLEDevice central = BLE.central();

	if (central)
	{
		if (!connected)
		{
			Serial.print("Connected to central: ");
			Serial.println(central.address());
		}

		if (central.connected())
		{
			connected = true;

			// Always process control characteristics (no authentication required)
			if (bleManager.getSpeedCharacteristic().written())
			{
				const auto value = bleManager.getSpeedCharacteristic().value();
				Serial.println("Speed characteristic written: " + String(value));
				speedMultiplier = value.toFloat() / 255.f * 20.f;
			}

			if (bleManager.getPatternIndexCharacteristic().written())
			{
				const auto value = bleManager.getPatternIndexCharacteristic().value();
				Serial.println("Pattern index characteristic written" + String(value));
				int val = value.toInt();
				Serial.println("Setting pattern index to: " + String(val));
				GoToPattern(val);
			}
			if (bleManager.getHighColorCharacteristic().written())
			{
				WavePlayer *currentWavePlayer = GetCurrentWavePlayer();
				if (currentWavePlayer)
				{
					UpdateColorFromCharacteristic(bleManager.getHighColorCharacteristic(), currentWavePlayer->hiLt, true);
				}
			}

			if (bleManager.getLowColorCharacteristic().written())
			{
				WavePlayer *currentWavePlayer = GetCurrentWavePlayer();
				if (currentWavePlayer)
				{
					UpdateColorFromCharacteristic(bleManager.getLowColorCharacteristic(), currentWavePlayer->loLt, false);
				}
			}

			if (bleManager.getLeftSeriesCoefficientsCharacteristic().written())
			{
				WavePlayer *currentWavePlayer = GetCurrentWavePlayer();
				if (currentWavePlayer)
				{
					Serial.println("Updating left series coefficients for current wave player");
					UpdateSeriesCoefficientsFromCharacteristic(bleManager.getLeftSeriesCoefficientsCharacteristic(), *currentWavePlayer);
				}
				else
				{
					Serial.println("No wave player available for series coefficients update");
				}
			}

			if (bleManager.getRightSeriesCoefficientsCharacteristic().written())
			{
				WavePlayer *currentWavePlayer = GetCurrentWavePlayer();
				if (currentWavePlayer)
				{
					Serial.println("Updating right series coefficients for current wave player");
					UpdateSeriesCoefficientsFromCharacteristic(bleManager.getRightSeriesCoefficientsCharacteristic(), *currentWavePlayer);
				}
				else
				{
					Serial.println("No wave player available for series coefficients update");
				}
			}

			if (bleManager.getCommandCharacteristic().written())
			{
				const auto value = bleManager.getCommandCharacteristic().value();
				Serial.println("Command characteristic written: " + String(value));
				ParseAndExecuteCommand(value);
			}
		}
		else
		{
			connected = false;
			Serial.println("Disconnected from central: ");
			Serial.println(central.address());
		}
	}
}

int loopCount = 0;
void loop()
{
	unsigned long ms = millis();
	FastLED.clear();
	Button::Event buttonEvent = pushButton.getEvent();
	Button::Event buttonEventSecondary = pushButtonSecondary.getEvent();

	// Primary button - pattern change
	if (buttonEvent == Button::Event::PRESS)
	{
		Serial.println("Primary button pressed");
		GoToNextPattern();
	}

	// Long press secondary button to enter pairing mode
	if (buttonEventSecondary == Button::Event::HOLD)
	{
		Serial.println("Secondary button long pressed - entering pairing mode");
	}

	// Handle BLE connections and authentication
	HandleBLE();

	Pattern_Loop();
	// CheckPotentiometers();
	// UpdateBrightnessInt(100);
	UpdateBrightnessPulse();

	// Heartbeat update
	unsigned long now = millis();
	if (now - lastHeartbeatSent > HEARTBEAT_INTERVAL_MS) {
		Serial.println("Sending heartbeat");
		bleManager.getHeartbeatCharacteristic().writeValue(now);
		lastHeartbeatSent = now;
	}

	last_ms = ms;
	FastLED.show();
	delay(1.f);

	bleManager.poll();
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
	else if (cmd == "fire_pattern")
	{
		// Command will look like fire_pattern:<idx>-<string-args>
		ParseFirePatternCommand(args);
	}
	else if (cmd == "ping") {
		// Echo back the timestamp
		Serial.println("Ping -- pong");
		bleManager.getCommandCharacteristic().writeValue("pong:" + args);
	}
	else
	{
		Serial.println("Unknown command: " + cmd);
	}
}

void StartBrightnessPulse(int targetBrightness, unsigned long duration)
{
	// Store current brightness before starting pulse
	previousBrightness = deviceState.brightness;
	pulseTargetBrightness = targetBrightness;
	pulseDuration = duration;
	pulseStartTime = millis();
	isPulsing = true;

	Serial.println("Starting brightness pulse from " + String(previousBrightness) + " to " + String(targetBrightness));
}

void UpdateBrightnessPulse()
{
	if (!isPulsing)
	{
		return;
	}

	unsigned long currentTime = millis();
	unsigned long elapsed = currentTime - pulseStartTime;

	if (elapsed >= pulseDuration)
	{
		// Pulse complete - return to previous brightness
		UpdateBrightnessInt(previousBrightness);
		isPulsing = false;
		Serial.println("Brightness pulse complete - returned to " + String(previousBrightness));
		return;
	}

	// Calculate current brightness using smooth interpolation
	float progress = (float) elapsed / (float) pulseDuration;

	// Use smooth sine wave interpolation for natural pulsing effect
	// This creates a full cycle: 0 -> 1 -> 0 over the duration
	float smoothProgress = (sin(progress * 2 * PI - PI / 2) + 1) / 2; // Full cycle: 0 to 1 to 0

	int currentBrightness = previousBrightness + (int) ((pulseTargetBrightness - previousBrightness) * smoothProgress);

	// Apply the calculated brightness
	UpdateBrightnessInt(currentBrightness);
}
