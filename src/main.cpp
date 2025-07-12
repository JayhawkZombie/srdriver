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

// Task classes - all in main.cpp where they work
class BLEUpdateTask {
public:
    BLEUpdateTask(BLEManager& manager) : bleManager(manager) {}
    void update() {
        bleManager.update();
    }
private:
    BLEManager& bleManager;
};

class FileStreamer {
public:
    FileStreamer() : active(false), file(), bufferSize(0) {}
    
    bool begin(const char* filename) {
        if (active) return false;
        file = SD.open(filename);
        if (!file) return false;
        active = true;
        bufferSize = 0;
        return true;
    }
    
    void update() {
        if (!active || !file) return;
        
        if (file.available()) {
            // Read a chunk of data
            int bytesRead = file.read(buffer, BUFFER_SIZE);
            if (bytesRead > 0) {
                bufferSize = bytesRead;
            }
        } else {
            // File is done
            file.close();
            active = false;
        }
    }
    
    bool isActive() const { return active; }
    const uint8_t* getBuffer() const { return buffer; }
    size_t getBufferSize() const { return bufferSize; }
    
private:
    static const size_t BUFFER_SIZE = 512;
    bool active;
    File file;
    uint8_t buffer[BUFFER_SIZE];
    size_t bufferSize;
};

class FileStreamAndPrint {
public:
    FileStreamAndPrint(FileStreamer& streamer, Task& streamTask)
        : streamer(streamer), streamTask(streamTask), started(false), printed(false) {
        currentFile[0] = '\0';
    }
    
    void start(const char* filename) {
        if (started && !printed) {
            Serial.println("ERROR: Print already in progress");
            return;
        }
        if (streamer.begin(filename)) {
            streamTask.enable();
            started = true;
            printed = false;
            Serial.print("Started streaming ");
            Serial.println(filename);
            strncpy(currentFile, filename, sizeof(currentFile)-1);
            currentFile[sizeof(currentFile)-1] = '\0';
        } else {
            Serial.print("ERROR: ");
            Serial.print(filename);
            Serial.println(" not found");
        }
    }
    
    void update() {
        if (started && !printed && !streamer.isActive() && streamer.getBuffer()) {
            Serial.print("----- FILE CONTENTS (" );
            Serial.print(currentFile);
            Serial.println(") BEGIN -----");
            Serial.write(streamer.getBuffer(), streamer.getBufferSize());
            Serial.println("\n----- FILE CONTENTS END -----");
            printed = true;
        }
    }
    
    bool isBusy() const { return started && !printed; }
    
private:
    FileStreamer& streamer;
    Task& streamTask;
    bool started;
    bool printed;
    char currentFile[64];
};

class SDCardIndexer {
public:
    struct FileEntry {
        String path;
        bool isDir;
        size_t size;
    };
    
    struct DirState {
        String path;
        uint8_t levels;
        File dir;
        DirState(String p, uint8_t l) : path(p), levels(l), dir() {}
    };
    
    static const size_t MAX_FILES = 100;
    
    SDCardIndexer() : active(false), finished(false), levels(0), fileCount(0) {}
    
    void begin(const char* rootDir, uint8_t maxLevels) {
        dirStack.clear();
        dirStack.push_back({String(rootDir), maxLevels});
        active = true;
        finished = false;
        fileCount = 0;
    }
    
    void update() {
        if (!active || dirStack.empty()) {
            active = false;
            finished = true;
            Serial.print("SDCard indexing DONE. Files indexed: ");
            Serial.println(fileCount);
            return;
        }
        
        DirState& current = dirStack.back();
        if (!current.dir) {
            current.dir = SD.open(current.path.c_str());
            if (!current.dir) {
                Serial.print("Failed to open directory: ");
                Serial.println(current.path);
                dirStack.pop_back();
                return;
            }
            if (!current.dir.isDirectory()) {
                Serial.print("Not a directory: ");
                Serial.println(current.path);
                current.dir.close();
                dirStack.pop_back();
                return;
            }
        }
        
        File entry = current.dir.openNextFile();
        if (entry) {
            if (fileCount >= MAX_FILES) {
                Serial.println("WARNING: File index cap reached, some files not indexed!");
                entry.close();
                current.dir.close();
                dirStack.clear();
                active = false;
                finished = true;
                return;
            }
            
            if (entry.isDirectory()) {
                fileList[fileCount++] = {String(entry.name()), true, 0};
                if (current.levels > 0) {
                    dirStack.push_back({String(entry.name()), static_cast<uint8_t>(current.levels - 1)});
                }
            } else {
                fileList[fileCount++] = {String(entry.name()), false, (size_t)entry.size()};
            }
            entry.close();
        } else {
            current.dir.close();
            dirStack.pop_back();
        }
    }
    
    bool isActive() const { return active; }
    bool isFinished() const { return finished; }
    size_t getFileCount() const { return fileCount; }
    const FileEntry& getFile(size_t idx) const { return fileList[idx]; }
    
private:
    bool active;
    bool finished;
    uint8_t levels;
    size_t fileCount;
    FileEntry fileList[MAX_FILES];
    std::vector<DirState> dirStack;
};

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

FileStreamAndPrint fileStreamAndPrint(fileStreamer, fileStreamTask);

SDCardIndexer sdCardIndexer;
Task sdCardIndexTask(1, TASK_FOREVER, [](){
    sdCardIndexer.update();
    if (!sdCardIndexer.isActive()) sdCardIndexTask.disable();
}, &runner, true); // Start enabled

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

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
	Serial.printf("Listing directory: %s\n", dirname);

	File root = fs.open(dirname);
	if (!root)
	{
		Serial.println("Failed to open directory");
		return;
	}
	if (!root.isDirectory())
	{
		Serial.println("Not a directory");
		return;
	}

	File file = root.openNextFile();
	while (file)
	{
		if (file.isDirectory())
		{
			Serial.print("  DIR : ");
			Serial.println(file.name());
			if (levels)
			{
				listDir(fs, file.name(), levels - 1);
			}
		}
		else
		{
			Serial.print("  FILE: ");
			Serial.print(file.name());
			Serial.print("\tSIZE: ");
			Serial.println(file.size());
		}
		file = root.openNextFile();
	}
}

void setup()
{
	wait_for_serial_connection();

	if (!SD.begin(SDCARD_PIN))
	{
		Serial.println("Failed to initialize SD card");
		// Not spinning, just don't do anything
	}

	listDir(SD, "/", 0);

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
	sdCardIndexer.begin("/", 2); // Start indexing SD card at setup
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
	if (sdCardIndexer.isFinished())
	{
		listFiles();
	}
	const auto now = millis();
	runner.execute();
	fileStreamAndPrint.update();
	// const auto runnerExecutionTime = now - last_ms;
	last_ms = now;
	// Serial.printf("Runner execution time: %dms\n", runnerExecutionTime);
	// Optionally, add a small delay if needed
	delay(1);

	// Serial command to trigger file streaming
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
		Serial.println("Received command: " + cmd);
        if (cmd.startsWith("PRINT ")) {
            String filename = cmd.substring(6);
            filename.trim();
            fileStreamAndPrint.start(filename.c_str());
        }
    }
}
