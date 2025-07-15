# FreeRTOS Abstraction Layer

This directory contains a clean C++ abstraction layer for FreeRTOS, designed to make task-based programming easier and more maintainable.

## Overview

The abstraction provides:
- **SRTask**: Base class for creating FreeRTOS tasks
- **SRQueue**: Template-based queue wrapper for inter-task communication
- **LogManager**: Global logging interface with SD card support
- **SDWriterTask**: Background task for writing logs to SD card

## Core Components

### SRTask (Base Task Class)

```cpp
class MyTask : public SRTask {
public:
    MyTask() : SRTask("MyTask", 4096, tskIDLE_PRIORITY + 1, 0) {}
    
protected:
    void run() override {
        // Your task logic here
        while (true) {
            // Do work
            SRTask::sleep(100);  // Sleep for 100ms
        }
    }
};
```

**Key Features:**
- Automatic task creation and cleanup
- Core pinning support
- Priority management
- Built-in sleep/yield methods

### SRQueue (Queue Wrapper)

```cpp
// Create a queue for integers
SRQueue<int> numberQueue(10, "Numbers");

// Send data (non-blocking)
numberQueue.send(42);

// Receive data (with timeout)
int value;
if (numberQueue.receive(value, 1000)) {  // 1 second timeout
    // Process value
}
```

**Key Features:**
- Type-safe template interface
- Timeout support
- Queue status monitoring
- Automatic cleanup

### LogManager (Global Logging)

```cpp
// Simple logging
LOG_INFO("System started");
LOG_ERROR("Something went wrong");
LOG_PRINTF("Value: %d", someValue);

// Or use the instance directly
LogManager::getInstance().debug("Debug message");
```

**Key Features:**
- Global singleton interface
- Multiple log levels (DEBUG, INFO, WARN, ERROR)
- Formatted output support
- Automatic queuing for SD card writing

## Usage Examples

### 1. Basic Task Creation

```cpp
#include "freertos/SRTask.h"

class BlinkTask : public SRTask {
public:
    BlinkTask() : SRTask("Blink", 2048, tskIDLE_PRIORITY + 1, 1) {}
    
protected:
    void run() override {
        pinMode(LED_BUILTIN, OUTPUT);
        
        while (true) {
            digitalWrite(LED_BUILTIN, HIGH);
            SRTask::sleep(500);
            digitalWrite(LED_BUILTIN, LOW);
            SRTask::sleep(500);
        }
    }
};

// In setup():
BlinkTask blinkTask;
blinkTask.start();
```

### 2. Inter-Task Communication

```cpp
#include "freertos/SRQueue.h"

// Define message structure
struct SensorData {
    float temperature;
    float humidity;
    uint32_t timestamp;
};

// Create queue
SRQueue<SensorData> sensorQueue(5, "SensorData");

// Producer task
class SensorTask : public SRTask {
protected:
    void run() override {
        while (true) {
            SensorData data = readSensor();
            sensorQueue.send(data);
            SRTask::sleep(1000);
        }
    }
};

// Consumer task
class DataProcessorTask : public SRTask {
protected:
    void run() override {
        SensorData data;
        while (true) {
            if (sensorQueue.receive(data, 5000)) {  // 5 second timeout
                processData(data);
            }
        }
    }
};
```

### 3. Logging System

```cpp
#include "freertos/LogManager.h"
#include "freertos/SDWriterTask.h"

// Create SD writer task
SDWriterTask sdWriter("/logs/app.log");

// Start the task
sdWriter.start();

// Now you can log anywhere in your code
LOG_INFO("Application started");
LOG_PRINTF("Temperature: %.2f°C", temperature);
LOG_ERROR("Failed to connect to WiFi");
```

## Core Assignment Strategy

### ESP32 Core Usage

- **Core 0**: WiFi/BLE stack, general tasks, system monitoring
- **Core 1**: User tasks, LED control, time-critical operations

### Priority Guidelines

- **tskIDLE_PRIORITY + 1**: Normal tasks
- **tskIDLE_PRIORITY + 2**: Important tasks (logging, SD card)
- **tskIDLE_PRIORITY + 3**: High priority tasks (LED updates)

## Migration from Arduino Loop

### Before (Arduino Style)
```cpp
void loop() {
    // Handle BLE
    bleManager.update();
    
    // Update LEDs
    ledManager.update();
    
    // Write logs
    if (millis() - lastLogTime > 1000) {
        writeLog();
        lastLogTime = millis();
    }
    
    // System monitoring
    if (millis() - lastStatusTime > 10000) {
        logStatus();
        lastStatusTime = millis();
    }
}
```

### After (FreeRTOS Style)
```cpp
// Each concern becomes its own task
BLEManagerTask bleTask;
LEDUpdateTask ledTask;
SDWriterTask logTask;
SystemMonitorTask monitorTask;

void setup() {
    // Start all tasks
    bleTask.start();      // Core 0, priority 1
    ledTask.start();      // Core 1, priority 3
    logTask.start();      // Core 0, priority 2
    monitorTask.start();  // Core 0, priority 1
}

void loop() {
    // Minimal orchestration or can be empty
    SRTask::sleep(1000);
}
```

## Best Practices

### 1. Task Design
- Keep tasks focused on a single responsibility
- Use queues for inter-task communication
- Avoid blocking operations in tasks
- Use appropriate stack sizes (2048-8192 bytes typical)

### 2. Memory Management
- Be careful with dynamic allocation in tasks
- Use fixed-size buffers when possible
- Monitor heap usage with system monitor task

### 3. Logging
- Use appropriate log levels
- Avoid excessive debug logging in production
- Let the SD writer task handle file I/O

### 4. Error Handling
- Always check queue send/receive return values
- Handle task creation failures
- Monitor task health with system monitor

## Integration with Existing Code

### Gradual Migration
1. Start with one subsystem (e.g., logging)
2. Create tasks for new features
3. Gradually move existing loop() logic to tasks
4. Keep Arduino loop() for compatibility during transition

### Compatibility
- All existing Arduino code continues to work
- FreeRTOS tasks run alongside Arduino loop()
- Can mix both approaches during migration

## Troubleshooting

### Common Issues

1. **Task Creation Fails**
   - Check stack size (too large or small)
   - Verify priority is within valid range
   - Ensure sufficient heap memory

2. **Queue Full/Empty**
   - Increase queue size if needed
   - Check producer/consumer timing
   - Monitor with system status

3. **Memory Issues**
   - Use system monitor to track heap
   - Reduce stack sizes if needed
   - Avoid large allocations in tasks

### Debugging
- Use LOG_DEBUG for task-specific debugging
- Monitor queue status with system monitor
- Check task priorities and core assignments 