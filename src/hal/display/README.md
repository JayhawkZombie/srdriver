# Display System Lifecycle

## Overview

The display system has a clear lifecycle that determines when tasks can safely use the `DisplayQueue` for banner messages.

## System States

### 1. STARTUP State
- **When**: During `setup()` before DisplayTask is created
- **Display**: Direct rendering via `ShowStartupStatusMessage()`
- **Queue**: Requests are ignored
- **Tasks**: Should not use DisplayQueue

### 2. READY State  
- **When**: DisplayTask has started and is running
- **Display**: Managed by DisplayTask with queue support
- **Queue**: Requests are accepted and processed
- **Tasks**: Can safely use DisplayQueue

### 3. ERROR State
- **When**: DisplayTask failed to start
- **Display**: Direct rendering only
- **Queue**: Requests are ignored
- **Tasks**: Should not use DisplayQueue

## Transition Points

### Setup Flow
```
1. DisplayQueue::setDisplayState(STARTUP)  // Line ~120 in main.cpp
2. ShowStartupStatusMessage("Starting")    // Direct rendering
3. ... various setup steps ...
4. ShowStartupStatusMessage("FreeRTOS Display")
5. g_displayTask = new DisplayTask()
6. g_displayTask->start()                  // Line ~272 in main.cpp
7. DisplayTask::run() sets state to READY  // Line ~15 in DisplayTask.cpp
8. delay(100)                              // Give task time to start
9. Log final state                          // Line ~290 in main.cpp
```

### When to Use DisplayQueue

**✅ SAFE - Use DisplayQueue:**
```cpp
DisplayQueue& queue = DisplayQueue::getInstance();

// Check if ready
if (queue.isDisplayTaskReady()) {
    queue.safeRequestBannerMessage(DisplayQueue::TASK_BLE, "Uploading...");
}
```

**❌ UNSAFE - Don't use during startup:**
```cpp
// During setup() - DisplayTask not ready yet
queue.requestBannerMessage(DisplayQueue::TASK_BLE, "Uploading..."); // Ignored!
```

## Best Practices

### 1. Always Check State
```cpp
DisplayQueue& queue = DisplayQueue::getInstance();

if (!queue.isDisplayTaskReady()) {
    Serial.println("Display not ready - skipping banner message");
    return;
}

// Now safe to use queue
queue.safeRequestBannerMessage(DisplayQueue::TASK_BLE, "Message");
```

### 2. Use Safe API Methods
```cpp
// ✅ Good - checks state automatically
bool success = queue.safeRequestBannerMessage(taskName, message);
if (success) {
    Serial.println("Banner message set");
} else {
    Serial.println("Display system not ready");
}

// ❌ Bad - doesn't check state
queue.requestBannerMessage(taskName, message); // May be ignored
```

### 3. Handle State Changes
```cpp
DisplayQueue::DisplayState state = queue.getDisplayState();
switch (state) {
    case DisplayQueue::DisplayState::STARTUP:
        // Still starting up - use direct rendering
        break;
    case DisplayQueue::DisplayState::READY:
        // Safe to use queue
        break;
    case DisplayQueue::DisplayState::ERROR:
        // Display system failed - fallback to serial
        break;
}
```

## Task Integration Examples

### BLE Task
```cpp
void BLEUpdateTask::showUploadProgress(int percent) {
    DisplayQueue& queue = DisplayQueue::getInstance();
    
    if (queue.isDisplayTaskReady()) {
        char message[32];
        snprintf(message, sizeof(message), "Upload %d%%", percent);
        queue.safeRequestBannerMessage(DisplayQueue::TASK_BLE, message);
    }
}
```

### System Monitor Task
```cpp
void SystemMonitorTask::showSystemStatus() {
    DisplayQueue& queue = DisplayQueue::getInstance();
    
    if (queue.isDisplayTaskReady()) {
        queue.safeRequestBannerMessage(DisplayQueue::TASK_SYSTEM, "System OK");
    }
}
```

## Debugging

### Check Display State
```cpp
void debugDisplayState() {
    DisplayQueue& queue = DisplayQueue::getInstance();
    
    Serial.printf("Display State: %d\n", (int)queue.getDisplayState());
    Serial.printf("Ready: %s\n", queue.isDisplayTaskReady() ? "Yes" : "No");
    Serial.printf("Has Message: %s\n", queue.hasActiveMessage() ? "Yes" : "No");
}
```

### Monitor in Loop
```cpp
void loop() {
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck > 5000) {  // Every 5 seconds
        DisplayQueue& queue = DisplayQueue::getInstance();
        if (!queue.isDisplayTaskReady()) {
            LOG_WARN("Display system not ready");
        }
        lastCheck = millis();
    }
}
```

## Future Enhancements

- **Priority System**: Allow high-priority messages to override low-priority ones
- **Multiple Messages**: Support for multiple concurrent messages
- **Message Timeouts**: Auto-clear messages after specified duration
- **Display Recovery**: Automatic restart of DisplayTask if it fails
- **Queue Statistics**: Track queue usage and performance metrics

## Message Timeouts

### How Timeouts Work

Messages can be set to automatically clear after a specified duration:

```cpp
DisplayQueue& queue = DisplayQueue::getInstance();

// Set timeout to 4 seconds
queue.setMessageTimeout(4000);

// Request a message that will auto-clear
queue.safeRequestBannerMessage(DisplayQueue::TASK_BLE, "Connected");
// Message will automatically disappear after 4 seconds
```

### Timeout Behavior

- **Global Setting**: `setMessageTimeout()` affects all future messages
- **Auto-Clear**: Messages are automatically cleared when timeout expires
- **DisplayTask Integration**: Timeout checking happens every display update cycle (~30 FPS)
- **Debug Logging**: Use `LOG_DEBUG` to see timeout events

### Debugging Timeouts

If messages aren't auto-clearing:

1. **Check if timeout is set**:
   ```cpp
   uint32_t timeout = queue.getMessageTimeout();
   Serial.printf("Current timeout: %d ms\n", timeout);
   ```

2. **Verify DisplayTask is running**:
   ```cpp
   if (!queue.isDisplayTaskReady()) {
       Serial.println("DisplayTask not running - timeouts won't work");
   }
   ```

3. **Monitor timeout events**:
   ```cpp
   // Enable debug logging to see timeout events
   // Look for: "Auto-clearing expired banner message"
   ```

4. **Test timeout functionality**:
   ```cpp
   // Use the test function from DisplayQueueExample.cpp
   testMessageTimeout();
   ```

### Common Issues

- **Timeout not set**: Default timeout is 0 (no auto-clear)
- **DisplayTask not running**: Timeout checking only happens when DisplayTask is active
- **Message replaced**: New messages reset the timeout timer
- **System time overflow**: Very long timeouts may have issues with `millis()` overflow

## Fade Effects

### Smooth Transitions

The display system supports smooth fade in/out effects for banner messages:

```cpp
DisplayQueue& queue = DisplayQueue::getInstance();

// Set fade durations (fade in: 500ms, fade out: 300ms)
queue.setFadeDuration(500, 300);

// Request a message with fade effects
queue.safeRequestBannerMessage(DisplayQueue::TASK_BLE, "Connected");
// Message will fade in over 500ms, then fade out over 300ms when cleared
```

### Fade Behavior

- **Fade In**: Messages start invisible and gradually become fully visible
- **Fade Out**: Messages gradually fade to invisible before being cleared
- **Smooth Animation**: Uses dithering patterns to simulate transparency on monochrome display
- **High Refresh Rate**: 30 FPS updates for smooth fade transitions
- **Configurable**: Different fade in/out durations can be set
- **Automatic**: Works with both manual clearing and timeout auto-clearing

### Fade States

```cpp
DisplayQueue::FadeState state = queue.getFadeState();
switch (state) {
    case DisplayQueue::FadeState::NONE:
        // No fade effect active
        break;
    case DisplayQueue::FadeState::FADE_IN:
        // Message is fading in (opacity increasing)
        break;
    case DisplayQueue::FadeState::SOLID:
        // Message is fully visible
        break;
    case DisplayQueue::FadeState::FADE_OUT:
        // Message is fading out (opacity decreasing)
        break;
}
```

### Opacity Monitoring

```cpp
uint8_t opacity = queue.getFadeOpacity(); // 0-255
Serial.printf("Current opacity: %d\n", opacity);
```

### Testing Fade Effects

```cpp
// Test basic fade functionality
testFadeEffects();

// Test timeout with fade
testTimeoutWithFade();
```

### Default Configuration

The system is configured with default fade durations in `main.cpp`:
- **Fade In**: 500ms
- **Fade Out**: 300ms

This provides smooth, visually appealing transitions for all banner messages. 