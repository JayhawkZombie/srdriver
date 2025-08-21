#pragma once

// Example integration for main.cpp
// Add these includes to main.cpp:

/*
#include "hal/HardwareInputTask.h"
#include "hal/InputEvent.h"
*/

// Add this to the global task instances section:
/*
static HardwareInputTask *g_hardwareInputTask = nullptr;
*/

// Add this to setup() function:
/*
// Initialize hardware input task
LOG_INFO("Initializing FreeRTOS hardware input task...");

g_hardwareInputTask = HardwareInputTaskBuilder()
    .addButton("mainButton", PUSHBUTTON_PIN)
    .addButton("secondaryButton", PUSHBUTTON_PIN_SECONDARY)
    .addPotentiometer("brightnessPot", POTENTIOMETER_PIN_BRIGHTNESS)
    .addPotentiometer("speedPot", POTENTIOMETER_PIN_SPEED)
    .addPotentiometer("extraPot", POTENTIOMETER_PIN_EXTRA)
    .build();

if (g_hardwareInputTask && g_hardwareInputTask->start()) {
    LOG_INFO("FreeRTOS hardware input task started");
    
    // Register callbacks
    g_hardwareInputTask->registerCallback("brightnessPot", InputEventType::POTENTIOMETER_CHANGE, 
        [](const InputEvent& event) {
            LOG_INFO("Brightness changed via hardware input");
            UpdateBrightness(event.mappedValue / 255.0f);
            bleManager.updateBrightness();
        });
    
    g_hardwareInputTask->registerCallback("speedPot", InputEventType::POTENTIOMETER_CHANGE,
        [](const InputEvent& event) {
            LOG_INFO("Speed changed via hardware input");
            // Use SpeedController instead of direct speedMultiplier assignment
            SpeedController* speedController = SpeedController::getInstance();
            if (speedController) {
                float newSpeed = event.mappedValue / 255.0f * 20.0f;
                speedController->setSpeed(newSpeed);
            }
        });
    
    g_hardwareInputTask->registerCallback("mainButton", InputEventType::BUTTON_PRESS,
        [](const InputEvent& event) {
            LOG_INFO("Main button pressed");
            // Handle main button press
        });
    
    // Global callback for logging all events
    g_hardwareInputTask->registerGlobalCallback([](const InputEvent& event) {
        LOG_DEBUGF("Input event: %s - %d (value: %d, mapped: %d)", 
                   event.deviceName.c_str(), 
                   static_cast<int>(event.eventType),
                   event.value, 
                   event.mappedValue);
    });
    
} else {
    LOG_ERROR("Failed to start FreeRTOS hardware input task");
    if (g_hardwareInputTask) {
        delete g_hardwareInputTask;
        g_hardwareInputTask = nullptr;
    }
}
*/

// Add this to cleanupFreeRTOSTasks():
/*
// Stop and cleanup hardware input task
if (g_hardwareInputTask) {
    g_hardwareInputTask->stop();
    delete g_hardwareInputTask;
    g_hardwareInputTask = nullptr;
    LOG_INFO("Hardware input task stopped");
}
*/

// Remove from loop():
/*
// OLD - Remove this from loop()
// CheckPotentiometers();  // REMOVE THIS
*/ 