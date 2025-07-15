#pragma once

#include "SRTask.h"
#include "LogManager.h"

/**
 * Simple LED blink task - demonstrates basic task usage
 */
class LEDBlinkTask : public SRTask {
public:
    LEDBlinkTask(uint8_t pin = LED_BUILTIN, uint32_t intervalMs = 1000)
        : SRTask("LEDBlink", 2048, tskIDLE_PRIORITY + 1, 1),  // Pin to core 1
          _pin(pin), _intervalMs(intervalMs) {}
    
protected:
    void run() override {
        pinMode(_pin, OUTPUT);
        LOG_INFO("LEDBlinkTask started");
        
        TickType_t lastWakeTime = xTaskGetTickCount();
        
        while (true) {
            digitalWrite(_pin, HIGH);
            LOG_DEBUG("LED ON");
            
            SRTask::sleepUntil(&lastWakeTime, _intervalMs / 2);
            
            digitalWrite(_pin, LOW);
            LOG_DEBUG("LED OFF");
            
            SRTask::sleepUntil(&lastWakeTime, _intervalMs / 2);
        }
    }
    
private:
    uint8_t _pin;
    uint32_t _intervalMs;
}; 