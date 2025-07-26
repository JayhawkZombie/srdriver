#define FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED 0
// #define FASTLED_RP2040_CLOCKLESS_PIO 0

#include <FastLED.h>
#include <stdint.h>
#include "Utils.hpp"
#include "Globals.h"
#include "../lights/LightPlayer2.h"
#include "../lights/DataPlayer.h"
#include "hal/input/buttons/Button.hpp"
#include "hal/input/potentiometers/Potentiometer.hpp"
#include "die.hpp"
#include "../lights/WavePlayer.h"

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

#include <array>
#include <memory>

// FreeRTOS includes
#include "freertos/SRTask.h"
#if SUPPORTS_SD_CARD
#include "freertos/LogManager.h"
#endif
#include "freertos/LEDUpdateTask.h"
#include "freertos/BLEUpdateTask.h"
#include "freertos/SystemMonitorTask.h"

#if SUPPORTS_SD_CARD
#include "utility/SDUtils.h"
#include "utility/OutputManager.h"
#include "hal/SDCardAPI.h"
#include "hal/FileParser.h"
#endif

#if SUPPORTS_DISPLAY
#include "hal/display/SSD_1306Component.h"
#include "freertos/DisplayTask.h"
SSD1306_Display display;
#endif
#include "config/JsonSettings.h"
#include "utility/StringUtils.h"


#include "freertos/HardwareInputTask.h"
#include "hal/input/InputEvent.h"

// Global FreeRTOS task instances
static LEDUpdateTask *g_ledUpdateTask = nullptr;
#if SUPPORTS_BLE
static BLEUpdateTask *g_bleUpdateTask = nullptr;
#endif
static SystemMonitorTask *g_systemMonitorTask = nullptr;
#if SUPPORTS_DISPLAY
static DisplayTask *g_displayTask = nullptr;
#endif

static HardwareInputTask *g_hardwareInputTask = nullptr;

// Global HAL instances
#if SUPPORTS_SD_CARD
SDCardController *g_sdCardController = nullptr;
#endif
JsonSettings settings("/config/settings.json");
bool settingsLoaded = false;

#if SUPPORTS_TEMPERATURE_SENSOR
DS18B20Component *g_temperatureSensor = nullptr;
#endif


#if FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED
Rgbw rgbw = Rgbw(
	kRGBWDefaultColorTemp,
	kRGBWExactColors,      // Mode
	W3                     // W-placement
);

typedef WS2812<LED_PIN, RGB> ControllerT;  // RGB mode must be RGB, no re-ordering allowed.
static RGBWEmulatedController<ControllerT, GRB> rgbwEmu(rgbw);  // ordering goes here.
#endif

CRGB leds[NUM_LEDS];

// Global SD card availability flag
#if SUPPORTS_SD_CARD
bool g_sdCardAvailable = false;
#endif

void OnSettingChanged(DeviceState &state)
{
	Serial.println("Device state changed");
	FastLED.setBrightness(state.brightness);
	SaveUserPreferences(state);
	// Optionally: save preferences, update UI, etc.
}

void ShowStartupStatusMessage(String message)
{
#if SUPPORTS_DISPLAY
	// Show into a buffer so we see: Startup: [message]
	char buffer[100];
	snprintf(buffer, sizeof(buffer), "Startup: %s", message.c_str());
	display.clear();
	display.setTextColor(COLOR_WHITE);
	display.setTextSize(1);
	display.printCentered(2, "SRDriver", 1);
	display.drawLine(0, 12, 128, 12, COLOR_WHITE);
	display.printAt(2, 20, buffer, 1);
	display.show();
#endif
}

void wait_for_serial()
{
	while (!Serial)
	{
		delay(100);
	}
}

// Skip setting brightness from user settings if we're using the hardware input task
bool skipBrightnessFromUserSettings = false;

void setup()
{
	// wait_for_serial();
	Serial.begin(9600);
	LOG_INFO("Beginning setup");

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
		LOG_WARN("SD card not available - continuing without SD card support");
	}
	else
	{
		LOG_INFO("SD card initialized successfully");
	}
#endif


	LOG_PRINTF("Platform: %s", PlatformFactory::getPlatformName());

	// Initialize SDCardAPI singleton
#if SUPPORTS_SD_CARD
	SDCardAPI::initialize();

	// Initialize SD card systems if available
	if (g_sdCardAvailable)
	{
		ShowStartupStatusMessage("SD Card Features");

		// Initialize SD card systems
		LOG_INFO("SRDriver starting up with SD card support");
	}
	else
	{
		LOG_INFO("SRDriver starting up (no SD card - logging to serial)");
	}
#endif

	ShowStartupStatusMessage("FreeRTOS Logging");

	// Initialize FreeRTOS logging system
	LOG_INFO("Initializing FreeRTOS logging system...");
#if SUPPORTS_SD_CARD
	LogManager::getInstance().initialize();
	LOG_INFO("FreeRTOS logging system started");

	// Test logging
	LOG_INFO("FreeRTOS logging system initialized");
	LOG_PRINTF("System started at: %d ms", millis());
	LOG_PRINTF("SD card available: %s", g_sdCardAvailable ? "yes" : "no");
	LOG_PRINTF("Platform: %s", PlatformFactory::getPlatformName());
#else
	LOG_INFO("FreeRTOS logging system started (SD card not supported)");
#endif

	g_hardwareInputTask = HardwareInputTaskBuilder()
		.fromJson("/data/hardwaredevices.json")
		.build();

	if (g_hardwareInputTask && g_hardwareInputTask->start())
	{
		bool foundMic = false;
		skipBrightnessFromUserSettings = true;
		LOG_INFO("Hardware input task started");

		if (g_hardwareInputTask->getDevice("mic"))
		{
			LOG_INFO("Microphone device found");
			foundMic = true;
		}
		else
		{
			LOG_ERROR("Microphone device not found");
			skipBrightnessFromUserSettings = false;
		}

		// Also register a device-wide callback to catch all mic events
		// We'll log them every 1000 events
		g_hardwareInputTask->getCallbackRegistry().registerDeviceCallback("mic", [](const InputEvent &event) {
			static int logLoopCount = 0;
			if (logLoopCount > 1000) {
				logLoopCount = 0;
				LOG_INFOF("🎤 MIC EVENT - Type: %d, Raw: %d, Mapped: %d", 
					static_cast<int>(event.eventType), event.value, event.mappedValue);
			}
			// LOG_INFOF("🎤 MIC EVENT - Type: %d, Raw: %d, Mapped: %d", 
			// 	static_cast<int>(event.eventType), event.value, event.mappedValue);
			// LOG_INFOF("🎤 MIC EVENT - Type: %d, Raw: %d, Mapped: %d",
			// 	static_cast<int>(event.eventType), event.value, event.mappedValue);
			// Update brightness to reflect volume level
			// Mapped value is in dB, so we need to convert it to a brightness value
			// 0dB is full brightness, -60dB is off
			// So we need to convert -60dB to 0dB
			// And then we need to convert 0dB to 255
			// And then we need to convert 255 to 0
			// And then we need to convert 0 to 255
			// And then we need to convert 255 to 0
			// So we need to convert -60dB to 0dB
			const int brightness = map(event.mappedValue, -60, 0, 0, 255);
			UpdateBrightnessInt(brightness);
			logLoopCount += 1;
		
		});

		// Add periodic ADC debugging
		g_hardwareInputTask->getCallbackRegistry().registerGlobalCallback([](const InputEvent &event) {
			static uint32_t lastDebugTime = 0;
			if (event.deviceName == "mic" && millis() - lastDebugTime > 5000) {
				lastDebugTime = millis();
				LOG_INFOF("🔍 Mic Debug - Raw ADC: %d, Mapped: %d, Type: %d", 
					event.value, event.mappedValue, static_cast<int>(event.eventType));
			}
		});

		LOG_INFO("Microphone callbacks registered");
	}
	else
	{
		LOG_ERROR("Failed to start hardware input task");
	}

#if SUPPORTS_SD_CARD
	// NOW we can load the settings??????
	LOG_DEBUG("Loading settings");
	settingsLoaded = settings.load();

	if (!settingsLoaded)
	{
		LOG_ERROR("Failed to load settings");
	}
#endif

#if SUPPORTS_DISPLAY
	if (settingsLoaded)
	{
		const auto displaySettings = settings._doc["display"];
		if (!displaySettings.isNull())
		{
			const auto addressSetting = displaySettings["address"];
			if (!addressSetting.isNull())
			{
				uint8_t address = hexToUint8(addressSetting.as<String>());
				LOG_DEBUGF("Found Display address: %d", address);

				display.setAddress(address);
				LOG_DEBUGF("Display address set to: %d", address);
			}
		}
	}

	display.setupDisplay();

	// Initialize DisplayQueue in STARTUP state
	DisplayQueue::getInstance().setDisplayState(DisplayQueue::DisplayState::STARTUP);

	// Immediately render to the display (Cannot use DisplayTask or DisplayQueue
	// yet because they are not initialized)
	ShowStartupStatusMessage("Starting");
#endif

#if SUPPORTS_BLE
	ShowStartupStatusMessage("BLE");
	if (!BLE.begin())
	{
		LOG_ERROR("Failed to initialize BLE");
		// Don't crash - continue without BLE
		LOG_WARN("Continuing without BLE support");
	}
	else
	{
		BLE.setLocalName("SRDriver");
		BLE.setDeviceName("SRDriver");
		BLE.advertise();
		LOG_INFO("BLE initialized");
	}
#else
	LOG_INFO("BLE not supported on this platform");
#endif

	// Used for RGB (NOT RGBW) LED strip
#if FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED
	FastLED.addLeds(&rgbwEmu, leds, NUM_LEDS);
#else
	FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
#endif
	FastLED.setBrightness(BRIGHTNESS);
	// Control power usage if computer is complaining/LEDs are misbehaving
	// FastLED.setMaxPowerInVoltsAndMilliamps(5, NUM_LEDS * 20);

	Pattern_Setup();

	// Initialize FreeRTOS LED update task
	LOG_INFO("Initializing FreeRTOS LED update task...");
	g_ledUpdateTask = new LEDUpdateTask(16);  // 60 FPS
	if (g_ledUpdateTask->start())
	{
		LOG_INFO("FreeRTOS LED update task started");
	}
	else
	{
		LOG_ERROR("Failed to start FreeRTOS LED update task");
	}

	// Add heartbeat characteristic
#if SUPPORTS_BLE
	bleManager.getHeartbeatCharacteristic().writeValue(millis());
#endif

	pinMode(PUSHBUTTON_PIN, INPUT_PULLUP);
	pinMode(PUSHBUTTON_PIN_SECONDARY, INPUT_PULLUP);

#if SUPPORTS_PREFERENCES
	prefsManager.begin();
	prefsManager.load(deviceState);
	prefsManager.save(deviceState);
	prefsManager.end();
	ApplyFromUserPreferences(deviceState, skipBrightnessFromUserSettings);
#else
	LOG_INFO("Preferences not supported on this platform - using defaults");
#endif


#if SUPPORTS_BLE
	bleManager.begin();
	bleManager.setOnSettingChanged(OnSettingChanged);
#endif

#if SUPPORTS_BLE
	// Initialize FreeRTOS BLE update task
	LOG_INFO("Initializing FreeRTOS BLE update task...");
	g_bleUpdateTask = new BLEUpdateTask(bleManager);
	if (g_bleUpdateTask->start())
	{
		LOG_INFO("FreeRTOS BLE update task started");
	}
	else
	{
		LOG_ERROR("Failed to start FreeRTOS BLE update task");
	}
#endif

	// Initialize FreeRTOS system monitor task
	LOG_INFO("Initializing FreeRTOS system monitor task...");
	g_systemMonitorTask = new SystemMonitorTask(15000);  // Every 15 seconds
	if (g_systemMonitorTask->start())
	{
		LOG_INFO("FreeRTOS system monitor task started");
	}
	else
	{
		LOG_ERROR("Failed to start FreeRTOS system monitor task");
	}

#if SUPPORTS_DISPLAY
	// Initialize FreeRTOS display task
	LOG_INFO("Initializing FreeRTOS display task...");
	g_displayTask = new DisplayTask(33);  // 30 FPS for smooth fade effects
	if (g_displayTask->start())
	{
		LOG_INFO("FreeRTOS display task started");
		// DisplayTask will set state to READY when it starts running
	}
	else
	{
		LOG_ERROR("Failed to start FreeRTOS display task");
		DisplayQueue::getInstance().setDisplayState(DisplayQueue::DisplayState::ERROR);
	}
#endif

	ShowStartupStatusMessage("Done");

	// Log the final display system state
	DisplayQueue::DisplayState finalState = DisplayQueue::getInstance().getDisplayState();
	switch (finalState)
	{
		case DisplayQueue::DisplayState::STARTUP:
			LOG_WARN("Display system still in STARTUP state - DisplayTask may not have started");
			break;
		case DisplayQueue::DisplayState::READY:
			LOG_INFO("Display system ready - queue requests now accepted");
			break;
		case DisplayQueue::DisplayState::ERROR:
			LOG_ERROR("Display system failed to start - queue requests will be ignored");
			break;
	}
}

/**
 * Clean up FreeRTOS tasks
 * Call this during shutdown or restart
 */
void cleanupFreeRTOSTasks()
{
	LOG_INFO("Shutting down FreeRTOS tasks...");

	// Stop and cleanup LED update task
	if (g_ledUpdateTask)
	{
		g_ledUpdateTask->stop();
		delete g_ledUpdateTask;
		g_ledUpdateTask = nullptr;
		LOG_INFO("LED update task stopped");
	}

	// Stop and cleanup BLE update task
#if SUPPORTS_BLE
	if (g_bleUpdateTask)
	{
		g_bleUpdateTask->stop();
		delete g_bleUpdateTask;
		g_bleUpdateTask = nullptr;
		LOG_INFO("BLE update task stopped");
	}
#endif

	// Stop and cleanup system monitor task
	if (g_systemMonitorTask)
	{
		g_systemMonitorTask->stop();
		delete g_systemMonitorTask;
		g_systemMonitorTask = nullptr;
		LOG_INFO("System monitor task stopped");
	}

	// Stop and cleanup display task
#if SUPPORTS_DISPLAY
	if (g_displayTask)
	{
		g_displayTask->stop();
		delete g_displayTask;
		g_displayTask = nullptr;
		LOG_INFO("Display task stopped");
	}
#endif

	// Cleanup SDCardAPI
#if SUPPORTS_SD_CARD
	SDCardAPI::cleanup();
	LOG_INFO("SDCardAPI cleaned up");
#endif

	// Stop and cleanup SD writer task (flush logs first)
#if SUPPORTS_SD_CARD
	// This section is removed as per the edit hint.
#endif

	LOG_INFO("FreeRTOS tasks cleanup complete");
}

void DrawError(const CRGB &color)
{
	for (int i = 0; i < LEDS_MATRIX_X; i += 2)
	{
		leds[i] = color;
	}
}

void loop()
{
	// Monitor FreeRTOS tasks every 5 seconds
	static unsigned long lastLogCheck = 0;
	const auto now = millis();
	if (now - lastLogCheck > 5000)
	{
		lastLogCheck = now;

		// Check FreeRTOS SD writer task
#if SUPPORTS_SD_CARD
		// This section is removed as per the edit hint.
#endif

		// Check FreeRTOS LED update task
		if (g_ledUpdateTask)
		{
			if (!g_ledUpdateTask->isRunning())
			{
				LOG_ERROR("FreeRTOS LED update task stopped unexpectedly");
			}
			else
			{
				LOG_DEBUGF("LED Update - Frames: %d, Interval: %d ms",
					g_ledUpdateTask->getFrameCount(),
					g_ledUpdateTask->getUpdateInterval());
			}
		}

		// Check FreeRTOS BLE update task
#if SUPPORTS_BLE
		if (g_bleUpdateTask && !g_bleUpdateTask->isRunning())
		{
			LOG_ERROR("FreeRTOS BLE update task stopped unexpectedly");
		}
#endif

		// Check FreeRTOS system monitor task
		if (g_systemMonitorTask && !g_systemMonitorTask->isRunning())
		{
			LOG_ERROR("FreeRTOS system monitor task stopped unexpectedly");
		}

		// Check FreeRTOS display task
#if SUPPORTS_DISPLAY
		if (g_displayTask)
		{
			if (!g_displayTask->isRunning())
			{
				LOG_ERROR("FreeRTOS display task stopped unexpectedly");
			}
			else
			{
				LOG_DEBUGF("Display Update - Frames: %d, Interval: %d ms",
					g_displayTask->getFrameCount(),
					g_displayTask->getUpdateInterval());

				// Check display performance
				if (!g_displayTask->isPerformanceAcceptable())
				{
					LOG_WARNF("Display performance issue: %s", g_displayTask->getPerformanceReport().c_str());
					LOG_INFO("Consider reducing display update frequency if performance issues persist");
				}
				else
				{
					LOG_DEBUGF("Display performance: %s", g_displayTask->getPerformanceReport().c_str());
				}
			}
		}
#endif

		// Log detailed task information every 30 seconds
		static unsigned long lastDetailedCheck = 0;
		if (now - lastDetailedCheck > 30000)
		{
			lastDetailedCheck = now;
			if (g_systemMonitorTask)
			{
				g_systemMonitorTask->logDetailedTaskInfo();
			}
		}
	}

	// Serial command to trigger file streaming
	if (Serial.available())
	{
		String cmd = Serial.readStringUntil('\n');
		cmd.trim();

		// Log the command using new FreeRTOS logging
		LOG_INFO("Serial command received: " + cmd);
		// Set output target to SERIAL_OUTPUT for commands received via serial
#if SUPPORTS_SD_CARD
		SDCardAPI::getInstance().setOutputTarget(OutputTarget::SERIAL_OUTPUT);
		SDCardAPI::getInstance().handleCommand(cmd);
		// Reset to BLE for future BLE commands
		SDCardAPI::getInstance().setOutputTarget(OutputTarget::BLE);
#endif

	}
}
