#pragma once

#include "SRTask.h"
#include "LogManager.h"
#include "PlatformConfig.h"
#include "PatternManager.h"  // For UpdatePattern and UpdateBrightnessPulse functions
#include "../lights/LEDManager.h"

#if SUPPORTS_LEDS
#include <FastLED.h>
#include "Globals.h"  // For NUM_LEDS
#include "LEDStorage.h"  // For leds array
#endif

// Forward declarations
#if SUPPORTS_LEDS
extern CRGB leds[];
#endif
extern Button pushButton;

/**
 * LEDUpdateTask - FreeRTOS task for LED pattern updates and rendering
 * 
 * Handles:
 * - Pattern updates and calculations
 * - FastLED rendering
 * - Button event processing
 * - Synchronization with shared LED data
 */
class LEDUpdateTask : public SRTask {
public:
    LEDUpdateTask(uint32_t updateIntervalMs = 16,  // ~60 FPS
                  uint32_t stackSize = 8192,
                  UBaseType_t priority = tskIDLE_PRIORITY + 3,  // High priority for smooth LED updates
                  BaseType_t core = 1)  // Pin to core 1 (away from WiFi/BLE)
        : SRTask("LEDUpdate", stackSize, priority, core),
          _updateIntervalMs(updateIntervalMs),
          _frameCount(0),
          _lastFpsLog(0),
          _maxPatternTime(0) {}
    
    /**
     * Get current frame count
     */
    uint32_t getFrameCount() const { return _frameCount; }
    
    /**
     * Get update interval
     */
    uint32_t getUpdateInterval() const { return _updateIntervalMs; }
    
    /**
     * Set update interval (for dynamic FPS adjustment)
     */
    void setUpdateInterval(uint32_t intervalMs) {
        _updateIntervalMs = intervalMs;
    }

    void setNumConfiguredLEDs(int numLEDs) {
        _numConfiguredLEDs = numLEDs;
    }

    int getNumConfiguredLEDs() const {
        return _numConfiguredLEDs;
    }

    /**
     * Initialize FastLED hardware (call before creating task)
     * This sets up the LED strip and blacks out all LEDs
     * Returns true if initialization successful, false otherwise
     */
    static bool initializeLEDs() {
#if SUPPORTS_LEDS
        return initializeFastLED();
#else
        return false;  // LEDs not supported
#endif
    }

protected:
    /**
     * Main task loop - handles LED pattern updates and rendering
     */
    void run() override {
        LOG_INFO("LED update task started");
        LOG_PRINTF("Update interval: %d ms (~%d FPS)", 
                   _updateIntervalMs, 1000 / _updateIntervalMs);
        
        TickType_t lastWakeTime = xTaskGetTickCount();
        static unsigned long lastUpdateTime = micros();

        if (!g_ledManager)
        {
            // No LEDs - just sleep
            TickType_t lastWakeTime = xTaskGetTickCount();
            while (true)
            {
                SRTask::sleepUntil(&lastWakeTime, _updateIntervalMs);
            }
        }
        
#if SUPPORTS_LEDS
        while (true) {
            if (isShuttingDown) {
                break;
            }
            FastLED.setBrightness(deviceState.brightness);
            // Measure pattern loop execution time
            uint32_t patternStart = micros();
            FastLED.clear();

            for (int i = 0; i < NUM_LEDS; i++) {
                LightArr[i] = Light(0, 0, 0);
                BlendLightArr[i] = Light(0, 0, 0);
                // FinalLeds[i] = Light(0, 0, 0);
            }

            const auto now = micros();
            const auto dt = now - lastUpdateTime;
            float dtSeconds = dt * 0.000001f;
            if (dt < 0)
            {
                dtSeconds = 0.0016f;  // Handle micros() overflow
            }
            lastUpdateTime = now;
            if (g_ledManager)
            {
                g_ledManager->safeProcessQueue();
                // g_ledManager->update(dtSeconds, LightArr, NUM_LEDS);
                g_ledManager->update(dtSeconds, LightArr, _numConfiguredLEDs);
                // g_ledManager->render(LightArr, NUM_LEDS);
                g_ledManager->render(LightArr, _numConfiguredLEDs);
            }


            // Pattern_Loop();
            
            // Copy LED data from LightArr to FastLED array
            extern Light LightArr[];
            for (int i = 0; i < NUM_LEDS; i++) {
                leds[i] = LightArr[i];
            }
            
            uint32_t patternEnd = micros();
            uint32_t patternTime = patternEnd - patternStart;
            
            if (isShuttingDown) {
                break;
            }
            FastLED.show();
            
            // Sleep until next frame
            SRTask::sleepUntil(&lastWakeTime, _updateIntervalMs);
        }
#else
        // LEDs not supported - just sleep forever
        while (true) {
            SRTask::sleepUntil(&lastWakeTime, _updateIntervalMs);
        }
#endif
    }

private:
    uint32_t _updateIntervalMs;
    uint32_t _frameCount;
    uint32_t _lastFpsLog;
    uint32_t _maxPatternTime;
    int _numConfiguredLEDs = NUM_LEDS;
    
    // Functions are now included from PatternManager.h
    // void UpdatePattern(Button::Event buttonEvent);
    // void UpdateBrightnessPulse();
}; 