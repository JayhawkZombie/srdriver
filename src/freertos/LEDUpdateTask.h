#pragma once

#include "SRTask.h"
#include "LogManager.h"
#include <FastLED.h>
#include "PatternManager.h"  // For UpdatePattern and UpdateBrightnessPulse functions
#include "Globals.h"  // For NUM_LEDS

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
        uint32_t lastTimingLog = 0;
        uint32_t totalPatternTime = 0;
        uint32_t patternTimeCount = 0;
        
        while (true) {
            FastLED.setBrightness(deviceState.brightness);
            // Measure pattern loop execution time
            uint32_t patternStart = micros();
            FastLED.clear();
            Pattern_Loop();
            
            // Copy LED data from LightArr to FastLED array
            extern Light LightArr[];
            for (int i = 0; i < NUM_LEDS; i++) {
                leds[i] = LightArr[i];
            }
            
            uint32_t patternEnd = micros();
            uint32_t patternTime = patternEnd - patternStart;
            
            // Accumulate timing data
            totalPatternTime += patternTime;
            patternTimeCount++;
            
            // Render to LEDs (FreeRTOS handles synchronization)
            FastLED.show();
            
            // Increment frame counter
            _frameCount++;
            
            // Log FPS and timing every 5 seconds
            uint32_t now = millis();
            if (now - _lastFpsLog > 5000) {
                uint32_t timeDiff = now - _lastFpsLog;
                if (timeDiff > 0) {  // Avoid division by zero
                    float fps = (float)_frameCount * 1000.0f / timeDiff;
                    LOG_DEBUGF("LED Update FPS: %.1f (frame %d)", fps, _frameCount);
                }
                
                // Log pattern timing statistics
                if (patternTimeCount > 0) {
                    uint32_t avgPatternTime = totalPatternTime / patternTimeCount;
                    LOG_DEBUGF("Pattern timing - Avg: %d μs, Max: %d μs, Count: %d", 
                              avgPatternTime, _maxPatternTime, patternTimeCount);
                    LOG_DEBUGF("Pattern time: %d μs", patternTime);
                }
                
                _frameCount = 0;
                _lastFpsLog = now;
                totalPatternTime = 0;
                patternTimeCount = 0;
                _maxPatternTime = 0;
            }
            
            // Track max pattern time
            if (patternTime > _maxPatternTime) {
                _maxPatternTime = patternTime;
            }
            
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