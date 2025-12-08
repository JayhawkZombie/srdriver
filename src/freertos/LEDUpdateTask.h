#pragma once

#include "SRTask.h"
#include "LogManager.h"
#include <FastLED.h>
#include "PatternManager.h"  // For UpdatePattern and UpdateBrightnessPulse functions
#include "Globals.h"  // For NUM_LEDS
#include "../lights/LEDManager.h"

// Forward declarations
extern CRGB leds[];
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
                g_ledManager->update(dtSeconds, LightArr, NUM_LEDS);
                // g_ledManager->render(LightArr, NUM_LEDS);
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
    }

private:
    uint32_t _updateIntervalMs;
    uint32_t _frameCount;
    uint32_t _lastFpsLog;
    uint32_t _maxPatternTime;
    
    // Functions are now included from PatternManager.h
    // void UpdatePattern(Button::Event buttonEvent);
    // void UpdateBrightnessPulse();
}; 