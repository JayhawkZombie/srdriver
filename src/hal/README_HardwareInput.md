# Hardware Input Task System

## Overview

The HardwareInputTask system provides a unified, FreeRTOS-based approach to polling and managing hardware inputs (buttons, potentiometers, microphones, etc.). It uses a generic device registry pattern with a flexible callback system for event handling.

## Architecture

### Core Components

1. **InputDevice Interface** - Base class for all input devices
2. **InputDeviceRegistry** - Manages collections of input devices by name
3. **InputCallbackRegistry** - Handles event callbacks with flexible registration
4. **HardwareInputTask** - FreeRTOS task that polls all devices and triggers events
5. **HardwareInputTaskBuilder** - Fluent interface for configuring the task

### Design Principles

- **Generic**: No device-specific callback names or interfaces
- **Flexible**: Support for JSON configuration and manual builder pattern
- **Event-driven**: Uses FreeRTOS queues for inter-task communication
- **Extensible**: Easy to add new device types without changing core interfaces

## Input Device Types

### Supported Devices

- **Button** - Digital input with press/hold/release detection
- **Potentiometer** - Analog input with hysteresis and mapping
- **Microphone** - Audio input with level detection and clipping detection

### Device Interface

```cpp
class InputDevice {
public:
    virtual ~InputDevice() = default;
    virtual void poll() = 0;
    virtual bool hasChanged() const = 0;
    virtual void resetChanged() = 0;
    virtual String getDeviceType() const = 0;
    virtual String getName() const = 0;
    virtual void* getEventData() = 0;
};
```

## Event System

### Event Types

```cpp
enum class InputEventType {
    BUTTON_PRESS,
    BUTTON_HOLD,
    BUTTON_RELEASE,
    POTENTIOMETER_CHANGE,
    MICROPHONE_AUDIO_DETECTED,
    MICROPHONE_CLIPPING,
    GENERIC_VALUE_CHANGE
};
```

### Event Structure

```cpp
struct InputEvent {
    String deviceName;
    InputEventType eventType;
    int value;           // Raw value
    int mappedValue;     // Mapped value (0-255, etc.)
    uint32_t timestamp;
    String eventData;    // JSON string for complex data
};
```

## Usage Examples

### Basic Setup

```cpp
// In main.cpp
static HardwareInputTask *g_hardwareInputTask = nullptr;

// In setup()
g_hardwareInputTask = HardwareInputTaskBuilder()
    .addButton("mainButton", PUSHBUTTON_PIN)
    .addButton("secondaryButton", PUSHBUTTON_PIN_SECONDARY)
    .addPotentiometer("brightnessPot", POTENTIOMETER_PIN_BRIGHTNESS)
    .addPotentiometer("speedPot", POTENTIOMETER_PIN_SPEED)
    .addPotentiometer("extraPot", POTENTIOMETER_PIN_EXTRA)
    .build();

if (g_hardwareInputTask && g_hardwareInputTask->start()) {
    LOG_INFO("FreeRTOS hardware input task started");
} else {
    LOG_ERROR("Failed to start FreeRTOS hardware input task");
}
```

### Callback Registration

```cpp
// Register callbacks for specific devices and events
g_hardwareInputTask->registerCallback("brightnessPot", InputEventType::POTENTIOMETER_CHANGE, 
    [](const InputEvent& event) {
        LOG_INFO("Brightness changed via hardware input");
        UpdateBrightness(event.mappedValue / 255.0f);
        bleManager.updateBrightness();
    });

g_hardwareInputTask->registerCallback("speedPot", InputEventType::POTENTIOMETER_CHANGE,
    [](const InputEvent& event) {
        LOG_INFO("Speed changed via hardware input");
        speedMultiplier = event.mappedValue / 255.0f * 20.0f;
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
```

### JSON Configuration

```json
{
  "inputDevices": [
    {
      "name": "mainButton",
      "type": "button",
      "pin": 2,
      "pollIntervalMs": 50
    },
    {
      "name": "brightnessPot",
      "type": "potentiometer",
      "pin": 4,
      "pollIntervalMs": 100
    },
    {
      "name": "speedPot",
      "type": "potentiometer",
      "pin": 5,
      "pollIntervalMs": 100
    },
    {
      "name": "mic",
      "type": "microphone",
      "pin": 0,
      "pollIntervalMs": 20
    }
  ]
}
```

```cpp
// Load configuration from JSON file
g_hardwareInputTask = HardwareInputTaskBuilder()
    .fromJsonFile("/config/input_devices.json")
    .build();
```

### Hybrid Configuration

```cpp
// Combine JSON config with runtime additions
g_hardwareInputTask = HardwareInputTaskBuilder()
    .fromJsonFile("/config/input_devices.json")  // Load from file
    .addButton("emergencyStop", 9, 25)           // Add runtime device
    .build();
```

## Task Integration

### FreeRTOS Task Management

```cpp
// In cleanupFreeRTOSTasks()
if (g_hardwareInputTask) {
    g_hardwareInputTask->stop();
    delete g_hardwareInputTask;
    g_hardwareInputTask = nullptr;
    LOG_INFO("Hardware input task stopped");
}
```

### Inter-Task Communication

```cpp
// Other tasks can subscribe to input events
QueueHandle_t inputQueue = g_hardwareInputTask->getInputEventQueue();
InputEvent event;

if (xQueueReceive(inputQueue, &event, pdMS_TO_TICKS(100)) == pdTRUE) {
    // Process input event
    switch (event.eventType) {
        case InputEventType::BUTTON_PRESS:
            // Handle button press
            break;
        case InputEventType::POTENTIOMETER_CHANGE:
            // Handle potentiometer change
            break;
    }
}
```

## Callback Registration Methods

### Specific Device and Event

```cpp
// Register callback for specific device and event type
g_hardwareInputTask->registerCallback("deviceName", InputEventType::EVENT_TYPE, callback);
```

### All Events for a Device

```cpp
// Register callback for all events from a specific device
g_hardwareInputTask->registerDeviceCallback("deviceName", callback);
```

### Global Callback

```cpp
// Register callback for all events from all devices
g_hardwareInputTask->registerGlobalCallback(callback);
```

## Device-Specific Details

### Button Input Device

- **Event Types**: `BUTTON_PRESS`, `BUTTON_HOLD`, `BUTTON_RELEASE`
- **Configuration**: Pin number, poll interval
- **Features**: Debouncing, hold detection, pull-up support

### Potentiometer Input Device

- **Event Type**: `POTENTIOMETER_CHANGE`
- **Configuration**: Pin number, poll interval, hysteresis threshold
- **Features**: Hysteresis filtering, value mapping, curve mapping

### Microphone Input Device

- **Event Types**: `MICROPHONE_AUDIO_DETECTED`, `MICROPHONE_CLIPPING`, `GENERIC_VALUE_CHANGE`
- **Configuration**: Pin number, poll interval, sample rate, threshold
- **Features**: Audio level detection, clipping detection, DC bias calibration

## Migration from Existing Code

### Replace CheckPotentiometers()

```cpp
// OLD - Remove from loop()
void loop() {
    // CheckPotentiometers();  // REMOVE THIS
    // ... rest of loop
}

// NEW - Handled by HardwareInputTask callbacks
```

### Replace Button Polling

```cpp
// OLD - Remove from loop()
void loop() {
    // Button polling code  // REMOVE THIS
    // ... rest of loop
}

// NEW - Handled by HardwareInputTask callbacks
```

## Performance Considerations

- **Polling Rate**: Default 20Hz (50ms interval), configurable per device
- **Memory Usage**: ~4KB stack per task, minimal heap usage
- **CPU Usage**: Minimal impact due to efficient polling and change detection
- **Queue Size**: Configurable event queue (default 20 events)

## Future Enhancements

### Planned Features

1. **Device Discovery** - Automatic detection of connected devices
2. **Dynamic Configuration** - Runtime device addition/removal
3. **Advanced Filtering** - Noise reduction, smoothing algorithms
4. **Calibration Support** - Device-specific calibration procedures
5. **Power Management** - Sleep modes for battery-powered operation

### Migration to unique_ptr

```cpp
// Future version
static std::unique_ptr<HardwareInputTask> g_hardwareInputTask;

// In setup()
g_hardwareInputTask = HardwareInputTaskBuilder()
    .addButton("mainButton", PUSHBUTTON_PIN)
    .buildUnique();  // Returns unique_ptr

// In cleanup - no manual delete needed
g_hardwareInputTask.reset();
```

## Troubleshooting

### Common Issues

1. **Task not starting**: Check pin assignments and device initialization
2. **No events received**: Verify callback registration and device polling
3. **High CPU usage**: Reduce polling frequency or optimize device implementations
4. **Queue overflow**: Increase queue size or reduce event frequency

### Debug Output

Enable debug logging to see device polling and event generation:

```cpp
// Global callback for debugging
g_hardwareInputTask->registerGlobalCallback([](const InputEvent& event) {
    Serial.printf("DEBUG: %s event - value: %d, mapped: %d\n", 
                  event.deviceName.c_str(), event.value, event.mappedValue);
});
```
