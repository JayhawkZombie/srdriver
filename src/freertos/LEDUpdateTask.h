#pragma once

#include "SRTask.h"
#include "LogManager.h"
#include <FastLED.h>
#include "PatternManager.h"  // For UpdatePattern and UpdateBrightnessPulse functions

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
          _lastFpsLog(0) {}
    
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
        
        while (true) {
            // Process button events
            Button::Event buttonEvent = pushButton.getEvent();
            
            // Update pattern (this modifies the leds[] array)
            UpdatePattern(buttonEvent);
            
            // Update brightness pulse
            UpdateBrightnessPulse();
            
            // Render to LEDs (this is the critical section)
            FastLED.show();
            
            // Increment frame counter
            _frameCount++;
            
            // Log FPS every 5 seconds
            uint32_t now = millis();
            if (now - _lastFpsLog > 5000) {
                float fps = (float)_frameCount * 1000.0f / (now - _lastFpsLog + 5000);
                LOG_DEBUGF("LED Update FPS: %.1f (frame %d)", fps, _frameCount);
                _frameCount = 0;
                _lastFpsLog = now;
            }
            
            // Sleep until next frame
            SRTask::sleepUntil(&lastWakeTime, _updateIntervalMs);
        }
    }

private:
    uint32_t _updateIntervalMs;
    uint32_t _frameCount;
    uint32_t _lastFpsLog;
    
    // Functions are now included from PatternManager.h
    // void UpdatePattern(Button::Event buttonEvent);
    // void UpdateBrightnessPulse();
}; 