# Device State Management Refactor

## Problem

Currently, device state is scattered across multiple components and BLE characteristic handlers, leading to:
- **State sync issues**: Brightness resets when speed changes
- **Inconsistent persistence**: Some state changes aren't saved
- **Complex BLE handlers**: Each handler must manage state updates manually
- **No single source of truth**: State can be modified from multiple places

## Solution: DeviceStateManager

Create a centralized `DeviceStateManager` singleton that provides a clean API for state management.

### Architecture

```
BLE Characteristic Handlers → DeviceStateManager → DeviceState + Persistence
                                    ↓
                            Component Notifications
```

### DeviceStateManager Class

```cpp
// DeviceStateManager.h
class DeviceStateManager {
private:
    static DeviceStateManager* instance;
    DeviceState currentState;
    std::vector<std::function<void(const DeviceState&)>> stateChangeCallbacks;
    
public:
    static DeviceStateManager* getInstance();
    static void initialize();
    static void destroy();
    
    // Clean API for BLE handlers
    void updateBrightness(int brightness);
    void updateSpeed(float speed);
    void updatePattern(int pattern);
    void updateHighColor(const Light& color);
    void updateLowColor(const Light& color);
    
    // Get current state
    const DeviceState& getState() const { return currentState; }
    
    // Register for state change notifications
    void onStateChanged(std::function<void(const DeviceState&)> callback);
    
    // Persistence
    void save();
    void load();
};
```

### Benefits

1. **Single Source of Truth**: All state changes go through DeviceStateManager
2. **Clean API**: BLE handlers just call `updateBrightness(32)` 
3. **Automatic Persistence**: State is saved whenever it changes
4. **Component Decoupling**: Components register for changes they care about
5. **No More State Sync Issues**: Brightness won't reset when speed changes

### Implementation Example

#### Before (Current)
```cpp
// BrightnessController.cpp - Complex handler
brightnessCharacteristicInfo.onWrite = [this](const unsigned char* value, size_t length) {
    char buf[16];
    size_t len = std::min(sizeof(buf) - 1, length);
    memcpy(buf, value, len);
    buf[len] = '\0';
    String s(buf);
    int rawVal = s.toInt();
    
    // Manual state management
    float mapped = getVaryingCurveMappedValue(rawVal / 255.0f, 3.f);
    int mappedVal = static_cast<int>(mapped * 255.0f + 0.5f);
    setBrightness(mappedVal);
    
    // Manual persistence
    if (brightnessCharacteristicInfo.characteristic) {
        brightnessCharacteristicInfo.characteristic->writeValue(String(mappedVal).c_str());
    }
};
```

#### After (Proposed)
```cpp
// BrightnessController.cpp - Simple handler
brightnessCharacteristicInfo.onWrite = [this](const unsigned char* value, size_t length) {
    char buf[16];
    size_t len = std::min(sizeof(buf) - 1, length);
    memcpy(buf, value, len);
    buf[len] = '\0';
    String s(buf);
    int rawVal = s.toInt();
    
    // Clean API - just update the state
    DeviceStateManager::getInstance()->updateBrightness(rawVal);
};
```

### Component Integration

```cpp
// PatternManager.cpp
void Pattern_Setup() {
    // Register for state changes
    DeviceStateManager::getInstance()->onStateChanged([](const DeviceState& state) {
        // Update pattern when state changes
        UpdateBrightnessInt(state.brightness);
        GoToPattern(state.patternIndex);
    });
}
```

### Implementation Steps

1. **Create DeviceStateManager** (singleton)
2. **Update BLE handlers** to use the clean API
3. **Register components** for state change notifications
4. **Remove direct state manipulation** from other parts of the code
5. **Test persistence** and state synchronization

### Migration Strategy

1. **Phase 1**: Create DeviceStateManager and migrate BrightnessController
2. **Phase 2**: Migrate Speed, Pattern, and Color characteristics
3. **Phase 3**: Update all components to use state change notifications
4. **Phase 4**: Remove old state management code

This refactor will make the system much more maintainable and solve the persistence issues currently being experienced.

## SpeedController Refactor

### Overview

The SpeedController will manage LED pattern speed independently, following the same pattern as BrightnessController. It will handle BLE characteristic registration, speed updates, and integration with the pattern system.

### SpeedController Class Structure

```cpp
// SpeedController.h
class SpeedController {
private:
    static SpeedController* instance;
    float currentSpeed;
    float targetSpeed;
    bool isTransitioning;
    unsigned long transitionStartTime;
    unsigned long transitionDuration;
    
    // BLE characteristic info
    BLECharacteristicInfo speedCharacteristicInfo;
    
public:
    // Singleton methods
    static SpeedController* getInstance();
    static void initialize();
    static void destroy();
    
    // Speed control
    void setSpeed(float speed);
    void setSpeedWithTransition(float targetSpeed, unsigned long duration);
    float getSpeed() const { return currentSpeed; }
    void stopTransition();
    
    // BLE integration
    void registerBLECharacteristic();
    void unregisterBLECharacteristic();
    
    // Update loop
    void update();
    
    // State management
    void syncWithDeviceState(DeviceState& deviceState);
    void updateDeviceState(DeviceState& deviceState);
    
private:
    SpeedController(); // Private constructor for singleton
    void updateSpeed(float newSpeed);
};
```

### BLE Characteristic Details

```cpp
// Speed characteristic configuration
speedCharacteristicInfo.characteristicUuid = "a5fb3bc5-9633-4b85-8a42-7756f11ef7ac";
speedCharacteristicInfo.descriptorUuid = "2901";
speedCharacteristicInfo.formatDescriptorUuid = "2904";
speedCharacteristicInfo.name = "Speed Control";
speedCharacteristicInfo.description = "Controls LED pattern speed (0.0-20.0)";
speedCharacteristicInfo.isWritable = true;
speedCharacteristicInfo.isReadable = true;
speedCharacteristicInfo.isNotifiable = true;
speedCharacteristicInfo.maxValueLength = 8; // "20.000" + null terminator
```

### Write Handler Implementation

```cpp
// SpeedController.cpp
speedCharacteristicInfo.onWrite = [this](const unsigned char* value, size_t length) {
    char buf[16];
    size_t len = std::min(sizeof(buf) - 1, length);
    memcpy(buf, value, len);
    buf[len] = '\0';
    String s(buf);
    float rawSpeed = s.toFloat();
    
    Serial.print("[Speed] Raw speed value: ");
    Serial.println(rawSpeed);
    
    // Clamp to valid range (0.0 to 20.0)
    float clampedSpeed = constrain(rawSpeed, 0.0f, 20.0f);
    
    Serial.print("[Speed] Clamped speed: ");
    Serial.println(clampedSpeed);
    
    // Update speed
    setSpeed(clampedSpeed);
    
    // Update the characteristic with the clamped value
    if (speedCharacteristicInfo.characteristic) {
        speedCharacteristicInfo.characteristic->writeValue(String(clampedSpeed, 3).c_str());
    }
};
```

### Integration Points

#### 1. PatternManager Integration
```cpp
// In PatternManager.cpp
void UpdatePattern() {
    // Get current speed from SpeedController
    SpeedController* speedController = SpeedController::getInstance();
    float currentSpeed = speedController ? speedController->getSpeed() : 1.0f;
    
    // Use speed in pattern updates
    testWavePlayer.update(dtSeconds * currentSpeed * speedMultiplier);
    rainbowPlayer.update(dtSeconds * currentSpeed);
    rainbowPlayer2.update(dtSeconds * currentSpeed);
}
```

#### 2. DeviceState Integration
```cpp
// In SpeedController.cpp
void SpeedController::setSpeed(float speed) {
    if (speed != currentSpeed) {
        currentSpeed = speed;
        
        // Update global speedMultiplier for backward compatibility
        extern float speedMultiplier;
        speedMultiplier = speed;
        
        // Update device state
        extern DeviceState deviceState;
        deviceState.speedMultiplier = speed;
        
        Serial.print("[Speed] Set to: ");
        Serial.println(speed);
    }
}
```

#### 3. BLE Registration
```cpp
// In main.cpp registerAllBLECharacteristics()
void registerAllBLECharacteristics() {
    // Initialize and register speed controller
    SpeedController::initialize();
    SpeedController* speedController = SpeedController::getInstance();
    if (speedController) {
        speedController->registerBLECharacteristic();
    }
}
```

### Migration Steps

1. **Create SpeedController files** (`SpeedController.h`, `SpeedController.cpp`)
2. **Implement singleton pattern** and basic speed management
3. **Add BLE characteristic registration** with proper write handler
4. **Update PatternManager** to use SpeedController instead of global `speedMultiplier`
5. **Remove speed handling** from BLEManager
6. **Test speed changes** via BLE and verify pattern updates
7. **Add to main.cpp** registration in `registerAllBLECharacteristics()`

### Benefits

- ✅ **Clean separation**: Speed logic isolated from BLEManager
- ✅ **Consistent API**: Same pattern as BrightnessController
- ✅ **Better range validation**: Proper clamping and validation
- ✅ **Smooth transitions**: Support for speed transitions (future enhancement)
- ✅ **Centralized state**: Speed managed in one place
- ✅ **BLE integration**: Clean characteristic registration and handling

### Future Enhancements

- **Speed transitions**: Smooth speed changes over time
- **Speed presets**: Predefined speed values (slow, medium, fast)
- **Speed curves**: Non-linear speed mapping
- **Speed synchronization**: Sync speed across multiple patterns
