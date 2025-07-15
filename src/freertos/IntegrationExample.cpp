/**
 * Integration Example - How to use FreeRTOS abstractions in existing code
 * 
 * This file shows how to integrate the FreeRTOS abstraction layer
 * into your existing Arduino/ESP32 project.
 */

#include "freertos/SRTask.h"
#include "freertos/SRQueue.h"
#include "freertos/LogManager.h"
#include "freertos/SDWriterTask.h"
#include "freertos/SystemMonitorTask.h"

// Global task instances (declare as extern in header if needed)
static SDWriterTask* g_sdWriterTask = nullptr;
static SystemMonitorTask* g_systemMonitorTask = nullptr;

/**
 * Initialize FreeRTOS tasks and logging system
 * Call this from your setup() function
 */
void initFreeRTOSSystem() {
    Serial.println("[FreeRTOS] Initializing task system...");
    
    // Create and start SD writer task (handles logging to SD card)
    g_sdWriterTask = new SDWriterTask("/logs/srdriver.log");
    if (g_sdWriterTask->start()) {
        Serial.println("[FreeRTOS] SD writer task started");
    } else {
        Serial.println("[FreeRTOS] Failed to start SD writer task");
    }
    
    // Create and start system monitor task
    g_systemMonitorTask = new SystemMonitorTask(15000);  // Every 15 seconds
    if (g_systemMonitorTask->start()) {
        Serial.println("[FreeRTOS] System monitor task started");
    } else {
        Serial.println("[FreeRTOS] Failed to start system monitor task");
    }
    
    // Wait a moment for tasks to initialize
    delay(100);
    
    // Log system startup
    LOG_INFO("FreeRTOS task system initialized");
    LOG_PRINTF("SD Writer: %s", g_sdWriterTask->isRunning() ? "Running" : "Failed");
    LOG_PRINTF("System Monitor: %s", g_systemMonitorTask->isRunning() ? "Running" : "Failed");
}

/**
 * Clean up FreeRTOS tasks
 * Call this from your cleanup/shutdown code
 */
void cleanupFreeRTOSSystem() {
    LOG_INFO("Shutting down FreeRTOS task system...");
    
    if (g_systemMonitorTask) {
        g_systemMonitorTask->stop();
        delete g_systemMonitorTask;
        g_systemMonitorTask = nullptr;
    }
    
    if (g_sdWriterTask) {
        g_sdWriterTask->forceFlush();  // Flush any pending logs
        g_sdWriterTask->stop();
        delete g_sdWriterTask;
        g_sdWriterTask = nullptr;
    }
    
    Serial.println("[FreeRTOS] Task system shutdown complete");
}

/**
 * Example of how to use logging throughout your existing code
 */
void exampleLoggingUsage() {
    // Basic logging
    LOG_INFO("This is an info message");
    LOG_WARN("This is a warning message");
    LOG_ERROR("This is an error message");
    LOG_DEBUG("This is a debug message");
    
    // Formatted logging
    int temperature = 25;
    float humidity = 60.5f;
    LOG_PRINTF("Temperature: %d°C, Humidity: %.1f%%", temperature, humidity);
    
    // Conditional logging
    if (temperature > 30) {
        LOG_WARNF("Temperature is high: %d°C", temperature);
    }
    
    // Error logging with context
    if (!SD.begin()) {
        LOG_ERROR("Failed to initialize SD card");
    } else {
        LOG_INFO("SD card initialized successfully");
    }
}

/**
 * Example of how to create a custom task for your specific needs
 */
class CustomDataProcessorTask : public SRTask {
public:
    CustomDataProcessorTask() 
        : SRTask("DataProcessor", 4096, tskIDLE_PRIORITY + 1, 0) {}
    
protected:
    void run() override {
        LOG_INFO("Custom data processor task started");
        
        TickType_t lastWakeTime = xTaskGetTickCount();
        
        while (true) {
            // Your custom processing logic here
            processData();
            
            // Log progress periodically
            LOG_DEBUG("Data processing cycle complete");
            
            // Sleep until next cycle (500ms)
            SRTask::sleepUntil(&lastWakeTime, 500);
        }
    }
    
private:
    void processData() {
        // Example processing logic
        // This could be your existing data processing code
        // moved from the main loop
    }
};

/**
 * Example of how to integrate with existing managers/classes
 */
void integrateWithExistingCode() {
    // Example: Replace existing logging calls
    // OLD: Serial.println("Processing command: PRINT");
    // NEW: LOG_INFO("Processing command: PRINT");
    
    // Example: Replace existing status reporting
    // OLD: Serial.printf("Heap: %d bytes\n", ESP.getFreeHeap());
    // NEW: LOG_PRINTF("Heap: %d bytes", ESP.getFreeHeap());
    
    // Example: Replace existing error handling
    // OLD: Serial.println("Error: Could not open file");
    // NEW: LOG_ERROR("Could not open file");
    
    // The system monitor task will automatically log system status
    // so you can remove manual status logging from your main loop
}

/**
 * Example of how to modify your existing main loop
 */
void exampleMainLoop() {
    // OLD: Everything in loop()
    // NEW: Minimal orchestration in loop(), tasks handle the work
    
    // Your existing setup code
    // initFreeRTOSSystem();  // Call this in setup()
    
    // Your existing loop code becomes much simpler
    while (true) {
        // Only keep high-level orchestration here
        // Let tasks handle the detailed work
        
        // Example: Check if tasks are still running
        if (g_sdWriterTask && !g_sdWriterTask->isRunning()) {
            LOG_ERROR("SD writer task stopped unexpectedly");
            // Maybe restart it or handle the error
        }
        
        // Sleep to allow other tasks to run
        SRTask::sleep(1000);
    }
}
