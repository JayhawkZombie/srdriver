#define FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED 0
// #define FASTLED_RP2040_CLOCKLESS_PIO 0

#include <stdint.h>
#include "Utils.hpp"
#include "PlatformConfig.h"

#if SUPPORTS_LEDS
#include "Globals.h"
#include "freertos/LEDStorage.h"
#endif

#include "hal/input/buttons/Button.hpp"
#include "hal/input/potentiometers/Potentiometer.hpp"
#include "die.hpp"

// Platform configuration and HAL
#include "PlatformConfig.h"
#include "hal/PlatformFactory.h"
#include "hal/SDCardController.h"

// Conditional includes based on platform support
#if SUPPORTS_BLE
#include <ArduinoBLE.h>
#endif

#include "GlobalState.h"
#include "DeviceState.h"
#include "hal/ble/BLEManager.h"
#include "PatternManager.h"
#include "UserPreferences.h"
#include "controllers/BrightnessController.h"
#include "controllers/SpeedController.h"

#include <array>
#include <memory>
#include <vector>

// FreeRTOS includes
#include "freertos/SRTask.h"
#if SUPPORTS_SD_CARD
#include "freertos/LogManager.h"
#endif
#if SUPPORTS_LEDS
#include "freertos/LEDUpdateTask.h"
#endif
#include "freertos/BLEUpdateTask.h"
#include "freertos/WiFiManager.h"
#include "freertos/SystemMonitorTask.h"
#include "freertos/TaskManager.h"

#if SUPPORTS_SD_CARD
#include "utility/SDUtils.h"
#include "utility/OutputManager.h"
#include "hal/SDCardAPI.h"
#include "hal/FileParser.h"
#endif

#if SUPPORTS_DISPLAY
#include "freertos/OLEDDisplayTask.h"
#endif
#include "config/JsonSettings.h"
#include "utility/StringUtils.h"


#include "freertos/HardwareInputTask.h"
#include "hal/input/InputEvent.h"
#include "lights/LEDManager.h"

// Global FreeRTOS task instances
#if SUPPORTS_BLE
#endif

HardwareInputTask *g_hardwareInputTask = nullptr;

// Global HAL instances
#if SUPPORTS_SD_CARD
SDCardController *g_sdCardController = nullptr;
#endif
JsonSettings settings("/config/settings.json");
bool settingsLoaded = false;

#if SUPPORTS_TEMPERATURE_SENSOR
DS18B20Component *g_temperatureSensor = nullptr;
#endif


// LED arrays moved to freertos/LEDStorage.cpp

// Global SD card availability flag
#if SUPPORTS_SD_CARD
bool g_sdCardAvailable = false;
#endif

#include "../hal/input/switchSPST.h"
int pinEncoderCLK = D4, pinEncoderDT = D3, pinEncoderSW = D2;
int lastClkState = HIGH;
void updateEncoder();

switchSPST rotEncButton;

volatile bool isShuttingDown = false;

void OnSettingChanged(DeviceState &state)
{
	Serial.println("Device state changed");
	LOG_INFOF_COMPONENT("Main", "Device state changed: brightness: %d", state.brightness);
#if SUPPORTS_LEDS
	FastLED.setBrightness(state.brightness);
#endif
	SaveUserPreferences(state);
	// Optionally: save preferences, update UI, etc.
}


void wait_for_serial()
{
	while (!Serial)
	{
		delay(100);
	}
}

void SetupOthers()
{
	pinMode(pinEncoderCLK, INPUT_PULLUP);
	pinMode(pinEncoderDT, INPUT_PULLUP);
	// pinMode(pinEncoderSW, INPUT_PULLUP);
	rotEncButton.init(pinEncoderSW, 0.3f, false);
	lastClkState = digitalRead(pinEncoderCLK);// initial value
	attachInterrupt(digitalPinToInterrupt(pinEncoderCLK), updateEncoder, CHANGE);
}

// We want a full rotation to be one change, but this is actually a change
// of 2 steps, a change occurs on a halfway step as well, which is in-between
// the notched points in the encoder.
volatile bool didChangeValue = false;
volatile int encoderValue = 0;
bool didChangeUp = false;

void updateEncoder()
{
	int currentClkState = digitalRead(pinEncoderCLK);
	int currentDtState = digitalRead(pinEncoderDT);
	const auto lastEncoderValue = encoderValue;

	if (currentClkState != lastClkState)
	{ // Check for a change in CLK
		if (currentDtState != currentClkState)
		{ // Determine direction
			encoderValue++;
			didChangeUp = true;
		}
		else
		{
			encoderValue--;
			didChangeUp = false;
		}
	}

	if (encoderValue != lastEncoderValue)
	{
		didChangeValue = true;
	}

	lastClkState = currentClkState;
}

int encoderBrightness = 128;

void UpdateBrightnessFromEncoder()
{
	BrightnessController *brightnessController = BrightnessController::getInstance();
	if (!brightnessController)
	{
		return;
	}

	int brightness = brightnessController->getBrightness();
	if (didChangeUp)
	{
		encoderBrightness++;
		encoderBrightness = constrain(encoderBrightness, 0, 255);
		brightnessController->updateBrightness(encoderBrightness);
	}
	else
	{
		encoderBrightness--;
		encoderBrightness = constrain(encoderBrightness, 0, 255);
		brightnessController->updateBrightness(encoderBrightness);
	}
}

// Order of effects to switch through when clicking the rotary encoder button
const std::vector<String> effectOrderJsonStrings = {
	// They're just effect commands, the same as would be sent to the WebSocket server
	{
		R"delimiter(
		{
			"t": "effect",
			"e": {
				"t": "white",
				"p": {
					"s": 1.0,
					"r": false,
					"d": -1
				}
			}
		}
		)delimiter",
	},
	{
		R"delimiter(
		{
			"t": "effect",
			"e": {
				"t": "rainbow",
				"p": {
					"s": 1.0,
					"r": false,
					"d": -1
				}
			}
		}
		)delimiter",
	},
	{
		R"delimiter(
		{
			"t": "effect",
			"e": {
				"t": "color_blend",
				"p": {
					"c1": "rgb(255,0,0)",
					"c2": "rgb(0,255,0)",
					"s": 1.0,
					"d": -1
				}
			}
		}
		)delimiter",
	},
	{
		R"delimiter(
		{
			"t": "effect",
			"e": {
				"t": "twinkle",
				"p": {
					"mnd": 0.5,
					"mxd": 5.6,
					"mns": 0.5,
					"mxs": 5.6,
					"sc": 0.4,
					"mb": 100,
					"fis": 4,
					"fos": 2
				}
			}
		}
		)delimiter",
	}
};
int currentEffectIndex = 0;

void TriggerNextEffect()
{
	currentEffectIndex++;
	if (currentEffectIndex >= effectOrderJsonStrings.size())
	{
		currentEffectIndex = 0;
	}
	LOG_DEBUGF_COMPONENT("Main", "Triggering next effect: (%i) %s", effectOrderJsonStrings[currentEffectIndex].length(), effectOrderJsonStrings[currentEffectIndex].c_str());
	String effectCommandJsonString = effectOrderJsonStrings[currentEffectIndex];
	HandleJSONCommand(effectCommandJsonString);
}

void LoopOthers(float dt)
{
	rotEncButton.update(dt);
	if (rotEncButton.pollEvent() == 1)
	{
		// PRESSED
		LOG_DEBUGF_COMPONENT("Main", "Rotary encoder button pressed");
		LOG_DEBUGF_COMPONENT("Main", "Current effect index: %d", currentEffectIndex);

		// Try triggering the next effect?
		TriggerNextEffect();
	}
	else if (rotEncButton.pollEvent() == -1)
	{
		// RELEASED
		LOG_DEBUGF_COMPONENT("Main", "Rotary encoder button released");
	}

	if (didChangeValue)
	{
		didChangeValue = false;
		LOG_DEBUGF_COMPONENT("Main", "Rotary encoder value changed: %d", encoderValue);
		UpdateBrightnessFromEncoder();
	}
}

// Skip setting brightness from user settings if we're using the hardware input task
bool skipBrightnessFromUserSettings = false;

// Function to register all BLE characteristics
void registerAllBLECharacteristics()
{
	BLEManager *ble = BLEManager::getInstance();
	if (!ble)
	{
		LOG_ERROR_COMPONENT("Startup", "BLE not available");
		return;
	}

	LOG_INFO_COMPONENT("Startup", "Registering all BLE characteristics...");

	// Initialize and register brightness controller
	LOG_INFO_COMPONENT("Startup", "Initializing BrightnessController...");
	BrightnessController::initialize();

	BrightnessController *brightnessController = BrightnessController::getInstance();
	if (brightnessController)
	{
		LOG_INFO_COMPONENT("Startup", "Brightness controller already initialized, registering characteristic...");
		brightnessController->registerBLECharacteristic();
		LOG_INFO_COMPONENT("Startup", "Brightness characteristic registration complete");
	}
	else
	{
		LOG_ERROR_COMPONENT("Startup", "Brightness controller not available for BLE registration");
	}

	// Initialize and register speed controller
	SpeedController::initialize();
	SpeedController *speedController = SpeedController::getInstance();
	if (speedController)
	{
		LOG_INFO_COMPONENT("Startup", "Speed controller initialized, registering characteristic...");
		speedController->registerBLECharacteristic();
		LOG_INFO_COMPONENT("Startup", "Speed controller characteristic registration complete");
	}
	else
	{
		LOG_ERROR_COMPONENT("Startup", "Failed to initialize speed controller");
	}

	// In the future, we can move these to their respective controllers
	// SpeedController::registerBLECharacteristics();
	// PatternController::registerBLECharacteristics();

	LOG_INFO_COMPONENT("Startup", "All characteristics registered");
}

void SerialAwarePowerLimiting()
{
#if SUPPORTS_LEDS
	// Many computers will complain about power consumption, and sometimes
	// will just outright disable the USB device, so we'll attempt to limit
	// power if we can detect a USB serial connection (not the best check,
	// since it'll miss most cases, but it's better than nothing)
	if (Serial)
	{
		FastLED.setMaxPowerInVoltsAndMilliamps(5, 1000);
	}
#endif
}

void OnShutdown()
{
	LOG_INFO_COMPONENT("Main", "OnShutdown called");
	isShuttingDown = true;
#if SUPPORTS_LEDS
	for (int i = 0; i < NUM_LEDS; i++) {
		leds[i] = CRGB::Black;
	}
	FastLED.setBrightness(0);
	FastLED.clear();
	FastLED.show();
	digitalWrite(LED_PIN, LOW);
#endif
	// esp_restart();
}

void setup()
{
	// wait_for_serial();
	
#if SUPPORTS_LEDS
	// Initialize LEDs early (black them out)
	LEDUpdateTask::initializeLEDs();
#else
	// LEDs not supported on this platform
	LOG_INFO_COMPONENT("Startup", "LED support disabled for this platform");
#endif

	esp_register_shutdown_handler(OnShutdown);

	Serial.begin(9600);
	SerialAwarePowerLimiting();
	SetupOthers();
	// LOG_INFO_COMPONENT("Startup", "Beginning setup");
	LOG_INFOF_COMPONENT("Startup", "Platform: %s", PlatformFactory::getPlatformName());

	// Initialize platform HAL
#if SUPPORTS_SD_CARD
	g_sdCardController = PlatformFactory::createSDCardController();
#endif

#if SUPPORTS_TEMPERATURE_SENSOR
	g_temperatureSensor = PlatformFactory::createTemperatureSensor(ONE_WIRE_BUS);
	g_temperatureSensor->begin();
#endif

	// Initialize SD card using HAL
#if SUPPORTS_SD_CARD
	g_sdCardAvailable = g_sdCardController->begin(SDCARD_PIN);
	if (!g_sdCardAvailable)
	{
		LOG_WARN_COMPONENT("Startup", "SD card not available - continuing without SD card support");
	}
	else
	{
		LOG_INFO_COMPONENT("Startup", "SD card initialized successfully");
	}

	SDCardAPI::initialize();
	delay(100);
#endif


	// Initialize FreeRTOS logging system
	// LOG_INFO_COMPONENT("Startup", "Initializing FreeRTOS logging system...");
#if SUPPORTS_SD_CARD
	LogManager::getInstance().initialize();

	// Configure log filtering (optional - can be enabled/disabled)
	// Uncomment the line below to show only WiFiManager logs:
	std::vector<String> logFilters = { "Main", "Startup", "WebSocketServer", "WiFiManager"	};
	LOG_SET_COMPONENT_FILTER(logFilters);

	// Uncomment the line below to show only new logs (filter out old ones):
	// LOG_SET_NEW_LOGS_ONLY();

	// LOG_INFO_COMPONENT("Startup", "FreeRTOS logging system initialized");
	// LOG_INFOF_COMPONENT("Startup", "System started at: %d ms", millis());
	// LOG_INFOF_COMPONENT("Startup", "SD card available: %s", g_sdCardAvailable ? "yes" : "no");
#else
	LOG_INFO_COMPONENT("Startup", "FreeRTOS logging system started (SD card not supported)");
#endif

	// g_hardwareInputTask = HardwareInputTaskBuilder()
	// 	.build();

	// if (g_hardwareInputTask && g_hardwareInputTask->start())
	// {
	// 	LOG_INFO_COMPONENT("Startup", "Hardware input task started");
	// }
	// else
	// {
	// 	LOG_ERROR_COMPONENT("Startup", "Failed to start hardware input task");
	// }

#if SUPPORTS_SD_CARD
	// NOW we can load the settings??????
	// LOG_DEBUG_COMPONENT("Startup", "Loading settings");
	settingsLoaded = settings.load();

	if (!settingsLoaded)
	{
		LOG_ERROR_COMPONENT("Startup", "Failed to load settings");
	}
#endif

	auto &taskMgr = TaskManager::getInstance();


#if SUPPORTS_BLE
	BLEManager *bleManager = nullptr;
	if (!BLE.begin())
	{
		LOG_ERROR_COMPONENT("Startup", "Failed to initialize BLE, continuing without BLE support");
	}
	else
	{
		BLE.setLocalName("SRDriver");
		BLE.setDeviceName("SRDriver");
		BLE.advertise();
		LOG_INFO_COMPONENT("Startup", "BLE initialized");

		BLEManager::initialize(deviceState, GoToPattern);
		bleManager = BLEManager::getInstance();

		if (bleManager)
		{
			// Register characteristics BEFORE starting BLE
			bleManager->registerCharacteristics();
			bleManager->getHeartbeatCharacteristic().writeValue(millis());
			// Register all additional BLE characteristics
			registerAllBLECharacteristics();
			// Now start BLE advertising
			bleManager->begin();
			bleManager->setOnSettingChanged(OnSettingChanged);

			// Initialize FreeRTOS BLE update task
			LOG_INFO_COMPONENT("Startup", "Initializing FreeRTOS BLE update task...");
			if (bleManager)
			{
				
				taskMgr.createBLETask(*bleManager);
			}
		}
		else
		{
			// LOG_ERROR_COMPONENT("Startup", "BLEManager is null!");
		}
	}
#else
	LOG_INFO_COMPONENT("Startup", "BLE not supported on this platform");
#endif

	// Initialize WiFi manager
	LOG_INFO_COMPONENT("Startup", "Initializing WiFi manager...");
	if (taskMgr.createWiFiManager(10))
	{
		LOG_INFO_COMPONENT("Startup", "WiFi manager started");
		// Set BLE manager reference if available
#if SUPPORTS_BLE
		if (bleManager)
		{
			if (auto* wifiMgr = taskMgr.getWiFiManager()) {
				wifiMgr->setBLEManager(bleManager);
				bleManager->setWiFiManager(wifiMgr);
			}
		}
#endif
	}
	else
	{
		LOG_ERROR_COMPONENT("Startup", "Failed to start WiFi manager");
	}

#if SUPPORTS_LEDS
	Pattern_Setup();
#endif

	extern LEDManager *g_ledManager;
	if (g_ledManager) {
		if (auto* wifiMgr = taskMgr.getWiFiManager()) {
			wifiMgr->setLEDManager(g_ledManager);

		if (settingsLoaded)
		{
			// Try to load panel configs from settings, we should have an array of them under "panels"
			if (settings._doc.containsKey("panels"))
			{
				bool usePanels = false;
				JsonObject panelsObj = settings._doc["panels"];
				if (panelsObj.containsKey("usePanels"))
				{
					usePanels = panelsObj["usePanels"].as<bool>();
				}
				std::vector<PanelConfig> panelConfigs;
				JsonArray panels = panelsObj["panelConfigs"];
				for (JsonVariant panel : panels)
				{
					PanelConfig panelConfig;
					panelConfig.rows = panel["rows"].as<int>();
					panelConfig.cols = panel["cols"].as<int>();
					panelConfig.row0 = panel["row0"].as<int>();
					panelConfig.col0 = panel["col0"].as<int>();
					panelConfig.type = panel["type"].as<int>();
					panelConfig.rotIdx = panel["rotIdx"].as<int>();
					panelConfig.swapTgtRCs = panel["swapTgtRCs"].as<bool>();
					panelConfigs.push_back(panelConfig);
				}
				if (usePanels)
				{
					g_ledManager->initPanels(panelConfigs);
				}
				// LOG_DEBUGF_COMPONENT("Startup", "Loaded panel configs: %d", panelConfigs.size());
			}

		}

			LOG_DEBUG_COMPONENT("Startup", "WiFiManager: LEDManager reference set for WebSocket");
		}
	}


#if SUPPORTS_PREFERENCES
	prefsManager.begin();
	prefsManager.load(deviceState);
	prefsManager.end();
	ApplyFromUserPreferences(deviceState, skipBrightnessFromUserSettings);
	encoderBrightness = deviceState.brightness;
	// Load WiFi credentials and attempt connection

	if (settingsLoaded) {
		if (auto* wifiMgr = taskMgr.getWiFiManager()) {
			std::vector<NetworkCredentials> knownNetworksList;
			if (settings._doc.containsKey("wifi"))
			{
				JsonObject wifiObj = settings._doc["wifi"];
				if (wifiObj.containsKey("knownNetworks"))
				{
					JsonArray knownNetworks = wifiObj["knownNetworks"];
					for (JsonVariant knownNetwork : knownNetworks)
					{
						NetworkCredentials networkCredentials;
						networkCredentials.ssid = knownNetwork["ssid"].as<String>();
						networkCredentials.password = knownNetwork["password"].as<String>();
						knownNetworksList.push_back(networkCredentials);
					}
				}
			}
			LOG_DEBUGF_COMPONENT("Startup", "Known networks loaded: %d", knownNetworksList.size());
			wifiMgr->setKnownNetworks(knownNetworksList);
			if (deviceState.wifiSSID.length() > 0) {
				LOG_DEBUGF_COMPONENT("Startup", "Setting credentials for '%s'", deviceState.wifiSSID.c_str());
				wifiMgr->setCredentials(deviceState.wifiSSID, deviceState.wifiPassword);
				LOG_DEBUG_COMPONENT("Startup", "WiFiManager: Calling checkSavedCredentials() to trigger auto-connect");
				wifiMgr->checkSavedCredentials();
			} else {
				LOG_DEBUGF_COMPONENT("Startup", "No WiFi credentials found - SSID length: %d", deviceState.wifiSSID.length());
			}
		}
	}

#else
	LOG_INFO("Preferences not supported on this platform - using defaults");
#endif

	// Initialize FreeRTOS LED update task
	// Note: Task can run even without SUPPORTS_LEDS - it will just sleep if no LED manager
	if (taskMgr.createLEDTask(16)) {  // 60 FPS
#if SUPPORTS_LEDS
		int numConfiguredLEDs = NUM_LEDS;
		if (settingsLoaded && settings._doc.containsKey("numLEDs"))
		{
			numConfiguredLEDs = settings._doc["numLEDs"].as<int>();
			LOG_DEBUGF_COMPONENT("Startup", "Setting numConfiguredLEDs to %d", numConfiguredLEDs);
			if (auto* ledTask = taskMgr.getLEDTask()) {
				ledTask->setNumConfiguredLEDs(numConfiguredLEDs);
			}
		}
		else
		{
			LOG_DEBUGF_COMPONENT("Startup", "No numLEDs found in settings, using default of %d", numConfiguredLEDs);
		}
#endif
	}

	// Initialize FreeRTOS system monitor task
	if (taskMgr.createSystemMonitorTask(1000)) {  // Every 1 second
#if SUPPORTS_POWER_SENSORS
		// Initialize power sensors in SystemMonitorTask
		if (auto* sysMon = taskMgr.getSystemMonitorTask()) {
			sysMon->initializePowerSensors(A2, A3, ACS712_30A, 5.0f, 3.3f);
#if ENABLE_POWER_SENSOR_CALIBRATION_DELAY
			// Force recalibration to ensure clean baseline
			sysMon->forceRecalibratePowerSensors();
#endif
		}
#endif
	}

	// Initialize FreeRTOS display task
	if (taskMgr.createOLEDDisplayTask(settingsLoaded ? &settings : nullptr, 200)) {
		LOG_INFO_COMPONENT("Startup", "FreeRTOS display task created and started");
	} else {
		LOG_ERROR_COMPONENT("Startup", "Failed to create FreeRTOS display task");
	}

	// WE're done!
	isBooting = false;
	LOG_INFO_COMPONENT("Startup", "Setup complete");
}

/**
 * Clean up FreeRTOS tasks
 * Call this during shutdown or restart
 */
void cleanupFreeRTOSTasks()
{
	// Clean up all tasks
	TaskManager::getInstance().cleanupAll();

	// Cleanup SDCardAPI
#if SUPPORTS_SD_CARD
	SDCardAPI::cleanup();
	// LOG_INFO("SDCardAPI cleaned up");
#endif
}

#if SUPPORTS_LEDS
void DrawError(const CRGB &color)
{
	for (int i = 0; i < LEDS_MATRIX_X; i += 2)
	{
		leds[i] = color;
	}
}
#endif

void loop()
{
	static unsigned long lastLoopTime = millis();
	const auto now = millis();
	const auto dt = now - lastLoopTime;
	lastLoopTime = now;
	float dtSeconds = dt * 0.001f;

	// Update brightness controller
	BrightnessController *brightnessController = BrightnessController::getInstance();
	if (brightnessController)
	{
		brightnessController->update();
	}

	// Update speed controller
	SpeedController *speedController = SpeedController::getInstance();
	if (speedController)
	{
		speedController->update();
	}

	// Monitor FreeRTOS tasks every 5 seconds
	static unsigned long lastLogCheck = 0;
	LoopOthers(0.16f);
	if (now - lastLogCheck > 5000)
	{
		lastLogCheck = now;
	}

	// Serial command to trigger file streaming
	if (Serial.available())
	{
		String cmd = Serial.readStringUntil('\n');
		cmd.trim();

		// Log the command using new FreeRTOS logging
		LOG_INFO("Serial command received: " + cmd);

		// Handle power sensor recalibration command
#if SUPPORTS_POWER_SENSORS
		if (cmd == "recalibrate_power")
		{
			LOG_INFO("Force recalibrating power sensors...");
			if (auto* sysMon = TaskManager::getInstance().getSystemMonitorTask())
			{
				if (sysMon->forceRecalibratePowerSensors())
				{
					LOG_INFO("Power sensor recalibration successful");
				}
				else
				{
					LOG_ERROR("Power sensor recalibration failed");
				}
			}
			else
			{
				LOG_ERROR("SystemMonitorTask not available for recalibration");
			}
			return; // Don't pass to SDCardAPI
		}

		// Handle clear calibration command
		if (cmd == "clear_calibration")
		{
			LOG_INFO("Clearing saved power sensor calibration...");
			Preferences prefs;
			if (prefs.begin("acs712_cal", false))
			{
				prefs.remove("midpoint");
				prefs.end();
				LOG_INFO("Saved calibration cleared - will recalibrate on next boot");
			}
			else
			{
				LOG_ERROR("Failed to clear calibration");
			}
			return; // Don't pass to SDCardAPI
		}
#else
		if (cmd == "recalibrate_power" || cmd == "clear_calibration")
		{
			// LOG_WARN("Power sensor commands not supported on this platform");
			return; // Don't pass to SDCardAPI
		}
#endif

		// Set output target to SERIAL_OUTPUT for commands received via serial
#if SUPPORTS_SD_CARD
		SDCardAPI::getInstance().setOutputTarget(OutputTarget::SERIAL_OUTPUT);
		SDCardAPI::getInstance().handleCommand(cmd);
		// Reset to BLE for future BLE commands
		SDCardAPI::getInstance().setOutputTarget(OutputTarget::BLE);
#endif
	}
}
