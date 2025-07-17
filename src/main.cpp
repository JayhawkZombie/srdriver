#define FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED 0
// #define FASTLED_RP2040_CLOCKLESS_PIO 0

#include <FastLED.h>
#include <stdint.h>
#include "Utils.hpp"
#include "Globals.h"
#include "../lights/LightPlayer2.h"
#include "../lights/DataPlayer.h"
#include "hal/Button.hpp"
#include "hal/Potentiometer.hpp"
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
#include "BLEManager.h"
#include "PatternManager.h"
#include "UserPreferences.h"

#if SUPPORTS_SD_CARD
#include <SD.h>
#endif

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
#include "SDCardAPI.h"
#include "hal/FileParser.h"
#endif

#if SUPPORTS_DISPLAY
#include "hal/SSD_1306Component.h"
#include "freertos/DisplayTask.h"
SSD1306_Display display;
#endif

// Global FreeRTOS task instances
static LEDUpdateTask *g_ledUpdateTask = nullptr;
#if SUPPORTS_BLE
static BLEUpdateTask *g_bleUpdateTask = nullptr;
#endif
static SystemMonitorTask *g_systemMonitorTask = nullptr;
#if SUPPORTS_DISPLAY
static DisplayTask *g_displayTask = nullptr;
#endif

// Global HAL instances
#if SUPPORTS_SD_CARD
SDCardController* g_sdCardController = nullptr;
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

// Function declarations for main.cpp specific functions
void CheckPotentiometers();

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

void setup()
{
	Serial.begin(9600);
	LOG_INFO("Beginning setup");
	LOG_PRINTF("Platform: %s", PlatformFactory::getPlatformName());

	// Initialize platform HAL
#if SUPPORTS_SD_CARD
	g_sdCardController = PlatformFactory::createSDCardController();
#endif

	// Platform abstraction tests removed - platform is working properly
	// No need for slow initialization tests during boot

#if SUPPORTS_DISPLAY
	display.setupDisplay();

	// Initialize DisplayQueue in STARTUP state
	DisplayQueue::getInstance().setDisplayState(DisplayQueue::DisplayState::STARTUP);

	// Immediately render to the display (Cannot use DisplayTask or DisplayQueue
	// yet because they are not initialized)
	ShowStartupStatusMessage("Starting");
#endif

	// Eventually will be more stuff here

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
	
	// SD card controller tests removed - SD card is working properly
	// No need for slow initialization tests during boot
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
#else
	LOG_INFO("Preferences not supported on this platform - using defaults");
#endif

	ApplyFromUserPreferences(deviceState);

#if SUPPORTS_BLE
	bleManager.begin();
	bleManager.setOnSettingChanged(OnSettingChanged);
#endif

#if SUPPORTS_DISPLAY
	ShowStartupStatusMessage("FreeRTOS Logging");
#endif
	
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

	ShowStartupStatusMessage("SDCardAPI");
	
	// Initialize SDCardAPI singleton
#if SUPPORTS_SD_CARD
	SDCardAPI::initialize();

	// Initialize SD card systems if available
	if (g_sdCardAvailable)
	{
		#if SUPPORTS_DISPLAY
		ShowStartupStatusMessage("SD Card Features");
		#endif
		
		// Initialize SD card systems
		LOG_INFO("SRDriver starting up with SD card support");
	}
	else
	{
		LOG_INFO("SRDriver starting up (no SD card - logging to serial)");
	}
#endif

	LOG_INFO("Device monitoring handled by FreeRTOS SystemMonitorTask");

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

int sharedCurrentIndexState = 0;
float speedMultiplier = 8.0f;

void CheckPotentiometers()
{
	// Always check brightness potentiometer regardless of potensControlColor
	// Call getValue() to update the change detection state
	brightnessPot.getValue();

	if (brightnessPot.hasChanged())
	{
		LOG_INFO("Brightness potentiometer has changed");
		float brightness = brightnessPot.getCurveMappedValue();
		UpdateBrightness(brightness);
		bleManager.updateBrightness();
		brightnessPot.resetChanged();
	}

	int speed = speedPot.getMappedValue(0, 255);
	int extra = extraPot.getMappedValue(0, 255);
	speedMultiplier = speed / 255.f * 20.f;
}

void loop()
{
	// Optionally, add a small delay if needed
	delay(1);

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

				// Add power efficiency monitoring
				// uint8_t powerScore = g_systemMonitorTask->getPowerEfficiencyScore();
				// LOG_PRINTF("Power Efficiency Score: %d/100", powerScore);

				// if (powerScore < 70) {
				// 	LOG_WARN("Low power efficiency detected - consider optimizations");
				// 	g_systemMonitorTask->suggestPowerOptimizations();
				// }
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
