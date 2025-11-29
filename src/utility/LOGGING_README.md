# SRDriver Logging System

The SRDriver firmware now includes a comprehensive logging system that allows you to easily log messages to the SD card without blocking the main application.

## Features

- **Non-blocking**: Logs are queued and written asynchronously
- **Configurable levels**: DEBUG, INFO, WARN, ERROR
- **Automatic file management**: Creates `/logs` directory and manages log files
- **Queue-based**: Prevents memory overflow with configurable queue size
- **Thread-safe**: Uses mutex protection for concurrent access

## Quick Start

### Basic Logging

```cpp
#include "utility/LogManager.h"

// Simple logging calls
LOG_INFO("Application started");
LOG_DEBUG("Processing data: " + String(dataValue));
LOG_WARN("Low memory condition detected");
LOG_ERROR("Failed to connect to device");
```

### Manual Logging (if you prefer)

```cpp
LogManager& logger = LogManager::getInstance();
logger.info("Manual info message");
logger.error("Manual error message");
```

## Configuration

### Log Level

Control which messages are logged:

```cpp
LogManager& logger = LogManager::getInstance();

// Only log WARN and ERROR messages
logger.setLogLevel(LogManager::WARN);

// Log everything including DEBUG
logger.setLogLevel(LogManager::DEBUG);
```

### Log File

Change the log file location:

```cpp
logger.setLogFile("/logs/myapp.log");
logger.setLogFile("/logs/errors.log"); // Different file for errors
```

### Queue Size

Control memory usage:

```cpp
logger.setMaxQueueSize(50); // Limit queue to 50 messages
```

## Log Format

Log entries are formatted as:
```
[timestamp] LEVEL: message
```

Example:
```
[12345] INFO: Application started
[12350] DEBUG: Processing command: LIST
[12355] INFO: File written successfully: /data/test.txt (1024 bytes)
[12360] ERROR: Failed to open file: /missing/file.txt
```

## Integration with Task Scheduler

The logging system integrates seamlessly with the existing TaskScheduler:

- **LogWriterTask**: Runs every 10ms when there are pending logs
- **Automatic activation**: Task enables itself when logs are queued
- **Efficient**: Processes one log entry per cycle to avoid blocking

## Usage Examples

### In SDCardAPI

```cpp
void SDCardAPI::writeFile(const String& filename, const String& content) {
    // ... existing code ...
    
    if (file) {
        file.print(content);
        file.close();
        LOG_INFO("File written successfully: " + filename + " (" + String(content.length()) + " bytes)");
    } else {
        LOG_ERROR("Failed to write file: " + filename);
    }
}
```

### In Main Loop

```cpp
void loop() {
    // ... existing code ...
    
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        LOG_INFO("Serial command received: " + cmd);
        // ... process command ...
    }
}
```

### Error Handling

```cpp
void someFunction() {
    if (!SD.begin(SDCARD_PIN)) {
        LOG_ERROR("Failed to initialize SD card");
        return;
    }
    
    LOG_INFO("SD card initialized successfully");
}
```

## Performance Considerations

- **Queue size**: Default 100 messages, adjust based on memory constraints
- **Write frequency**: One log entry per 10ms task cycle
- **File operations**: Each log entry opens/closes the file (consider batching for high-frequency logging)

## Troubleshooting

### Logs not appearing
1. Check that SD card is properly initialized
2. Verify `/logs` directory exists
3. Check log level setting
4. Monitor queue size: `logger.getQueueSize()`

### Memory issues
1. Reduce queue size: `logger.setMaxQueueSize(20)`
2. Increase log level to reduce message volume
3. Check for memory leaks in log messages

### File system issues
1. Ensure SD card has sufficient space
2. Check file permissions
3. Verify SD card is not write-protected

## Advanced Usage

### Custom Log Levels

You can extend the system with custom levels:

```cpp
// In LogManager.h, add new level
enum LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    CRITICAL = 4  // New level
};

// Add corresponding method
void critical(const String& message) {
    log(CRITICAL, message);
}
```

### Batch Logging

For high-frequency logging, consider batching:

```cpp
// Instead of logging every iteration
for (int i = 0; i < 1000; i++) {
    // ... process data ...
    if (i % 100 == 0) {  // Log every 100th iteration
        LOG_DEBUG("Processed " + String(i) + " items");
    }
}
```

The logging system is now ready to use throughout the SRDriver firmware! 🎉 