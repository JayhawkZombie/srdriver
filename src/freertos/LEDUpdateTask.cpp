#include "LEDUpdateTask.h"
#include "../lights/LEDManager.h"
#include "../GlobalState.h"
#include "../controllers/BrightnessController.h"
#include "../Utils.hpp"
#include "freertos/LogManager.h"

void LEDUpdateTask::run()
{
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
    while (true)
    {
        if (isShuttingDown)
        {
            break;
        }
        // Don't set brightness here - BrightnessController handles it during pulses
        // FastLED.setBrightness(deviceState.brightness);
        // Measure pattern loop execution time
        uint32_t patternStart = micros();
        FastLED.clear();

        for (int i = 0; i < NUM_LEDS; i++)
        {
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
        for (int i = 0; i < NUM_LEDS; i++)
        {
            leds[i] = LightArr[i];
        }

        uint32_t patternEnd = micros();
        uint32_t patternTime = patternEnd - patternStart;

        if (isShuttingDown)
        {
            break;
        }
        
        // Update brightness controller (handles pulse animations) right before show()
        // This ensures brightness is updated in the same task that calls show()
        // update() will call setBrightness() which sets FastLED.setBrightness() with the correct mapped value
        BrightnessController* bc = BrightnessController::getInstance();
        if (bc) {
            bc->update();  // Updates pulse animation and calls setBrightness() -> FastLED.setBrightness()
        }
        
        FastLED.show();

        // Sleep until next frame
        SRTask::sleepUntil(&lastWakeTime, _updateIntervalMs);
    }
#else
    // LEDs not supported - just sleep forever
    while (true)
    {
        SRTask::sleepUntil(&lastWakeTime, _updateIntervalMs);
    }
#endif
}
