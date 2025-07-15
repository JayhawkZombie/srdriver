// #define FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED 0
// #define FASTLED_RP2040_CLOCKLESS_PIO 0

#include <FastLED.h>
#include <stdint.h>
#include "Utils.hpp"
#include "Globals.h"
#include "LightPlayer2.h"
#include "DataPlayer.h"
#include "hal/Button.hpp"
#include "hal/Potentiometer.hpp"
#include "die.hpp"
#include "WavePlayer.h"
#include <ArduinoBLE.h>

#include "GlobalState.h"
#include "DeviceState.h"
#include "BLEManager.h"
#include "PatternManager.h"
#include "UserPreferences.h"

#include <SD.h>
#include <TaskScheduler.h>
#include <array>
#include <memory>

#include "tasks/LEDUpdateTask.h"
#include "tasks/BLEUpdateTask.h"
#include "tasks/FileStreamer.h"
#include "tasks/SDCardIndexer.h"
#include "tasks/LogWriterTask.h"
#include "tasks/DeviceMonitor.h"

#include "utility/SDUtils.h"
#include "utility/OutputManager.h"
#include "utility/LogManager.h"

#include "SDCardAPI.h"
#include "utility/FileParser.h"

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
void EnterSettingsMode();
void MoveToNextSetting();
void ExitSettingsMode();
void UpdateLEDsForSettings(int potentiometerValue);
void CheckPotentiometers();
void HandleBLE();

// Heartbeat timing
unsigned long lastHeartbeatSent = 0;
const unsigned long HEARTBEAT_INTERVAL_MS = 5000;

// Scheduler instance
Scheduler runner;

// Create task instances
LEDUpdateTask ledUpdateTask;
Task ledTask(33, TASK_FOREVER, [](){ ledUpdateTask.update(); }, &runner, true);

BLEUpdateTask bleUpdateTask(bleManager);
Task bleTask(10, TASK_FOREVER, [](){ bleUpdateTask.update(); }, &runner, true);

FileStreamer fileStreamer;
Task fileStreamTask(5, TASK_FOREVER, [](){
    fileStreamer.update();
    if (!fileStreamer.isActive()) fileStreamTask.disable();
}, &runner, false); // Start disabled

SDCardIndexer sdCardIndexer;
Task sdCardIndexTask(1, TASK_FOREVER, [](){
    sdCardIndexer.update();
    if (!sdCardIndexer.isActive()) sdCardIndexTask.disable();
}, &runner, true); // Start enabled

LogWriterTask logWriterTask;
Task logWriterTaskInstance(10000, TASK_FOREVER, [](){ 
    logWriterTask.update(); 
}, &runner, true); // Always enabled

DeviceMonitor deviceMonitor;
Task deviceMonitorTask(1000, TASK_FOREVER, [](){ 
    deviceMonitor.update(); 
}, &runner, true); // Run every second, always enabled

// Create a callback function to enable the file stream task
auto enableFileStreamTask = []() { fileStreamTask.enable(); };

// Task classes - all in main.cpp where they work

SDCardAPI sdCardAPI(fileStreamer, sdCardIndexer, enableFileStreamTask);

// Global SD card availability flag
bool g_sdCardAvailable = false;

void wait_for_serial_connection()
{
	uint32_t timeout_end = millis() + 2000;
	Serial.begin(9600);
	while (!Serial && timeout_end > millis()) {}  //wait until the connection to the PC is established
}

void OnSettingChanged(DeviceState &state)
{
	Serial.println("Device state changed");
	FastLED.setBrightness(state.brightness);
	SaveUserPreferences(state);
	// Optionally: save preferences, update UI, etc.
}

void setup()
{
	wait_for_serial_connection();

	g_sdCardAvailable = SD.begin(SDCARD_PIN);
	if (!g_sdCardAvailable)
	{
		Serial.println("Failed to initialize SD card - continuing without SD card support");
	} else {
		Serial.println("SD card initialized successfully");
	}

	// Test FileParser with /dads.txt only if SD card is available
	if (g_sdCardAvailable) {
		Serial.println("[Test] Reading from /dads.txt using FileParser...");
		FileParser input("/dads.txt", FileParser::Mode::READ);
		if (input.isOpen()) {
			int x;
			float y;
			String message;
			int z;
			input >> x >> y >> message >> z;
			Serial.print("[Test] x: "); Serial.println(x);
			Serial.print("[Test] y: "); Serial.println(y);
			Serial.print("[Test] message: "); Serial.println(message);
			Serial.print("[Test] z: "); Serial.println(z);
			input.close();
		} else {
			Serial.println("[Test] Failed to open /dads.txt");
		}

		listDir(SD, "/", 0);
	}

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

	prefsManager.begin();
	prefsManager.load(deviceState);
	prefsManager.save(deviceState);
	prefsManager.end();

	ApplyFromUserPreferences(deviceState);

	bleManager.begin();
	bleManager.setOnSettingChanged(OnSettingChanged);
	
	// Initialize logging system (works with or without SD card)
	Serial.println("[Main] Initializing logging system...");
	logWriterTask.begin();
	LogManager::getInstance().setLogFile("/logs/srdriver.log");
	LogManager::getInstance().setLogLevel(LogManager::INFO);
	
	if (g_sdCardAvailable) {
		// Initialize SD card systems
		sdCardIndexer.begin("/", 2); // Start indexing SD card at setup
		
		// Rotate the log file on startup (archive old one, start fresh)
		LogManager::getInstance().rotateLogFile();
		
		// Refresh the log writer task to use the new log file
		logWriterTask.refreshLogFile();
		
		LogManager::getInstance().info("SRDriver starting up");
		Serial.println("[Main] Logging system initialized with SD card support");
	} else {
		LogManager::getInstance().info("SRDriver starting up (no SD card - logging to serial)");
		Serial.println("[Main] Logging system initialized without SD card support");
	}
	
	// Initialize device monitoring (works with or without SD card)
	deviceMonitor.begin();
	deviceMonitor.setInterval(30000); // Monitor every 30 seconds
	Serial.println("[Main] Device monitor initialized");
	
	runner.startNow();
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

bool hasListedFiles = false;
void listFiles()
{
	if (hasListedFiles)
		{
		return;
	}
	for (size_t i = 0; i < sdCardIndexer.getFileCount(); i++)
	{
		if (sdCardIndexer.getFile(i).isDir)
		{
			Serial.println("DIR");
		}
		else
		{
			Serial.println("FILE");
		}
		Serial.println(sdCardIndexer.getFile(i).path);
	}
	hasListedFiles = true;
}

void loop()
{
	if (g_sdCardAvailable && sdCardIndexer.isFinished())
	{
		listFiles();
	}
	const auto now = millis();
	runner.execute();
	sdCardAPI.update();
	// const auto runnerExecutionTime = now - last_ms;
	last_ms = now;
	// Serial.printf("Runner execution time: %dms\n", runnerExecutionTime);
	// Optionally, add a small delay if needed
	delay(1);

	// Debug: check logging system status every 5 seconds
	static unsigned long lastLogCheck = 0;
	if (now - lastLogCheck > 5000) {
		lastLogCheck = now;
		LogManager& logger = LogManager::getInstance();
		Serial.print("[Main] Log system status - Queue size: ");
		Serial.print(logger.getQueueSize());
		Serial.print(", Task active: ");
		Serial.print(logWriterTask.isActive());
		Serial.print(", Log file: ");
		Serial.println(logger.getLogFile());
	}

	// Serial command to trigger file streaming
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
		cmd.trim();
		Serial.println("Received command: " + cmd);
        
        // Log the command
        LOG_INFO("Serial command received: " + cmd);
        
        // Set output target to SERIAL_OUTPUT for commands received via serial
        sdCardAPI.setOutputTarget(OutputTarget::SERIAL_OUTPUT);
        sdCardAPI.handleCommand(cmd);
        // Reset to BLE for future BLE commands
        sdCardAPI.setOutputTarget(OutputTarget::BLE);
    }
}
