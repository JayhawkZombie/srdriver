# Quick Start: FreeRTOS Logging & SD Writer Integration

This guide shows you how to quickly integrate the FreeRTOS logging and SD writer system into your existing code with minimal changes.

## Step 1: Add Includes to Your Main File

Add these includes to your `main.cpp`:

```cpp
#include "freertos/SRTask.h"
#include "freertos/LogManager.h"
#include "freertos/SDWriterTask.h"
```

## Step 2: Initialize the System

Add this to your `setup()` function, after SD card initialization:

```cpp
// Global SD writer task instance
static SDWriterTask* g_sdWriterTask = nullptr;

void setup() {
    // ... your existing setup code ...
    
    // Initialize SD card first
    if (!SD.begin()) {
        Serial.println("Failed to initialize SD card");
        return;
    }
    
    // Initialize FreeRTOS logging system
    Serial.println("[FreeRTOS] Initializing logging system...");
    g_sdWriterTask = new SDWriterTask("/logs/srdriver.log");
    if (g_sdWriterTask->start()) {
        Serial.println("[FreeRTOS] Logging system started");
        
        // Wait for task to initialize
        delay(100);
        
        // Test logging
        LOG_INFO("FreeRTOS logging system initialized");
        LOG_PRINTF("System started at: %d ms", millis());
        
    } else {
        Serial.println("[FreeRTOS] Failed to start logging system");
    }
    
    // ... rest of your setup code ...
}
```

## Step 3: Replace Your Existing Logging

Replace your existing `Serial.println` and `Serial.printf` calls with the new logging macros:

### Before:
```cpp
Serial.println("Processing command: PRINT");
Serial.printf("File size: %d bytes\n", fileSize);
Serial.println("Error: Could not open file");
```

### After:
```cpp
LOG_INFO("Processing command: PRINT");
LOG_PRINTF("File size: %d bytes", fileSize);
LOG_ERROR("Could not open file");
```

## Step 4: Use File Operations (Optional)

If you want to use the task-based file writing:

```cpp
// Write a file
if (g_sdWriterTask->writeFile("/data/test.txt", "Hello World")) {
    LOG_INFO("File write queued");
}

// Append to a file
if (g_sdWriterTask->appendFile("/logs/events.log", "User pressed button\n")) {
    LOG_DEBUG("Event logged");
}

// Create a new file
g_sdWriterTask->createFile("/data/new_file.txt");
```

## Step 5: Monitor the System (Optional)

Add this to your `loop()` function to monitor the logging system:

```cpp
void loop() {
    // ... your existing loop code ...
    
    // Monitor logging system (every 30 seconds)
    static uint32_t lastMonitorTime = 0;
    if (millis() - lastMonitorTime > 30000) {
        if (g_sdWriterTask && !g_sdWriterTask->isRunning()) {
            LOG_ERROR("SD writer task stopped unexpectedly");
        }
        lastMonitorTime = millis();
    }
    
    // ... rest of your loop code ...
}
```

## Step 6: Clean Up (Optional)

Add cleanup to your shutdown code:

```cpp
void cleanup() {
    if (g_sdWriterTask) {
        LOG_INFO("Shutting down logging system...");
        g_sdWriterTask->forceFlush();
        g_sdWriterTask->stop();
        delete g_sdWriterTask;
        g_sdWriterTask = nullptr;
    }
}
```

## What You Get

### ✅ **Immediate Benefits:**
- All your logs automatically go to `/logs/srdriver.log` on the SD card
- No more blocking SD card writes in your main loop
- Better system responsiveness
- Organized, timestamped logging

### ✅ **Log Levels Available:**
```cpp
LOG_DEBUG("Debug information");
LOG_INFO("General information");
LOG_WARN("Warning message");
LOG_ERROR("Error message");
LOG_PRINTF("Formatted: %d, %.2f", value1, value2);
```

### ✅ **File Operations Available:**
```cpp
g_sdWriterTask->writeFile(filename, data);      // Write new file
g_sdWriterTask->appendFile(filename, data);     // Append to file
g_sdWriterTask->createFile(filename);           // Create empty file
g_sdWriterTask->deleteFile(filename);           // Delete file
```

## Example: Replace Your SDCardAPI Logging

### Before (in SDCardAPI.cpp):
```cpp
void SDCardAPI::printFile(const String& filename) {
    Serial.printf("[SDCardAPI] Printing file: %s\n", filename.c_str());
    
    File file = SD.open(filename.c_str(), FILE_READ);
    if (!file) {
        Serial.printf("[SDCardAPI] Error: Could not open file %s\n", filename.c_str());
        return;
    }
    
    Serial.printf("[SDCardAPI] File size: %d bytes\n", file.size());
    // ... rest of function
}
```

### After (in SDCardAPI.cpp):
```cpp
void SDCardAPI::printFile(const String& filename) {
    LOG_PRINTF("Printing file: %s", filename.c_str());
    
    File file = SD.open(filename.c_str(), FILE_READ);
    if (!file) {
        LOG_ERRORF("Could not open file %s", filename.c_str());
        return;
    }
    
    LOG_PRINTF("File size: %d bytes", file.size());
    // ... rest of function
}
```

## Example: Replace Your BLEManager Logging

### Before (in BLEManager.cpp):
```cpp
void BLEManager::handleCommand(const String& command) {
    Serial.printf("[BLE Manager] Received command: %s\n", command.c_str());
    
    if (command.startsWith("PRINT")) {
        Serial.println("[BLE Manager] Processing PRINT command");
        // ... process command
    }
}
```

### After (in BLEManager.cpp):
```cpp
void BLEManager::handleCommand(const String& command) {
    LOG_PRINTF("Received command: %s", command.c_str());
    
    if (command.startsWith("PRINT")) {
        LOG_INFO("Processing PRINT command");
        // ... process command
    }
}
```

## Troubleshooting

### **Logs not appearing on SD card?**
- Check that SD card is initialized before starting the task
- Verify the `/logs/` directory exists
- Check that the task started successfully

### **System seems slower?**
- The task runs on core 0, which also handles WiFi/BLE
- Consider reducing log frequency or buffer size
- Monitor queue status with the system monitor

### **Memory issues?**
- Reduce buffer size in SDWriterTask constructor
- Reduce queue sizes
- Monitor heap usage with system monitor task

## Next Steps

Once this basic integration is working:

1. **Add the system monitor task** for automatic health reporting
2. **Create tasks for your other subsystems** (BLE, LED, etc.)
3. **Use file operations** for BLE file uploads/downloads
4. **Monitor and tune** the system performance

The beauty is that you can do this **gradually** - your existing code continues to work while you add FreeRTOS features one by one! 