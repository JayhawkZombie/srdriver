# SRDriver Board Support Strategy

## Overview

SRDriver is designed to support multiple microcontroller platforms through a Hardware Abstraction Layer (HAL) approach, inspired by Unreal Engine's platform abstraction patterns. This allows the core application logic to remain unchanged while supporting different hardware platforms.

## Supported Platforms

### Currently Supported
- **ESP32-S3** (Arduino Nano ESP32) - Primary platform
  - Dual-core ARM Cortex-M7
  - 8MB PSRAM, 16MB Flash
  - Built-in WiFi and BLE
  - 240MHz CPU frequency

### Planned Support
- **RP2040** (Raspberry Pi Pico) - Secondary platform
  - Dual-core ARM Cortex-M0+
  - 264KB SRAM, 2MB Flash
  - No built-in WiFi (external module required)
  - 133MHz CPU frequency

## Architecture Overview

### HAL (Hardware Abstraction Layer) Design

The system uses compile-time platform selection with abstract interfaces:

```cpp
// Example HAL interface
class LEDController {
public:
    virtual void begin() = 0;
    virtual void setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void show() = 0;
    virtual void clear() = 0;
};

// Platform-specific implementations
class ESP32LEDController : public LEDController { /* ESP32 implementation */ };
class RP2040LEDController : public LEDController { /* RP2040 implementation */ };
```

### Platform Factory Pattern

```cpp
class PlatformFactory {
public:
    static LEDController* createLEDController() {
#ifdef PLATFORM_ESP32
        return new ESP32LEDController();
#elif defined(PLATFORM_RP2040)
        return new RP2040LEDController();
#endif
    }
    
    static BLEController* createBLEController() {
#ifdef PLATFORM_ESP32
        return new ESP32BLEController();
#elif defined(PLATFORM_RP2040)
        return new RP2040BLEController();
#endif
    }
};
```

## Incremental Support Strategy

### Phase 1: LED Support Only (1-2 days)
**Goal**: Basic LED pattern functionality
- ✅ FastLED library (works on both platforms)
- ✅ LED pattern rendering
- ❌ No BLE, display, or system monitoring

**Files to create**:
- `hal/LEDController.h`
- `hal/ESP32LEDController.h/cpp`
- `hal/RP2040LEDController.h/cpp`
- `hal/PlatformFactory.h`

### Phase 2: BLE Support (3-5 days)
**Goal**: Add wireless communication
- ✅ LED patterns
- ✅ BLE connectivity and control
- ❌ No display or system monitoring

**BLE Library Differences**:
```cpp
// ESP32
#include <ArduinoBLE.h>
BLE.begin();
BLE.advertise();

// RP2040
#include <BLE.h>
BLE.begin();
BLE.advertise();
```

**Files to create**:
- `hal/BLEController.h`
- `hal/ESP32BLEController.h/cpp`
- `hal/RP2040BLEController.h/cpp`

### Phase 3: Display Support (2-3 days)
**Goal**: Add OLED display functionality
- ✅ LED patterns
- ✅ BLE connectivity
- ✅ Display rendering
- ❌ No system monitoring

**Display Library**: Adafruit SSD1306 (works on both platforms)

**Files to create**:
- `hal/DisplayController.h`
- `hal/ESP32DisplayController.h/cpp`
- `hal/RP2040DisplayController.h/cpp`

### Phase 4: System Monitoring (2-3 days)
**Goal**: Add system health monitoring
- ✅ Full feature set
- ✅ Platform-specific system APIs

**System API Differences**:
```cpp
// ESP32
ESP.getFreeHeap()
ESP.getCpuFreqMHz()

// RP2040
rp2040.getFreeHeap()
// No direct CPU freq API - use constant
```

**Files to create**:
- `hal/SystemController.h`
- `hal/ESP32SystemController.h/cpp`
- `hal/RP2040SystemController.h/cpp`

## Platform-Specific Challenges

### ESP32-S3 Advantages
- **Built-in WiFi**: No external module needed
- **Rich system APIs**: `ESP.getFreeHeap()`, `ESP.getCpuFreqMHz()`
- **Preferences library**: Persistent storage in flash
- **Core pinning**: `xTaskCreatePinnedToCore()`
- **Higher performance**: 240MHz vs 133MHz

### RP2040 Advantages
- **Lower cost**: ~$4 vs ~$15
- **Built-in microphone**: Audio input capability
- **Standard FreeRTOS**: More portable
- **PIO state machines**: Advanced I/O capabilities

### RP2040 Limitations
- **No WiFi**: Requires external module
- **Less RAM**: 264KB vs 8MB+ PSRAM
- **Slower CPU**: 133MHz vs 240MHz
- **Different BLE stack**: Requires different library
- **No Preferences library**: Must use LittleFS or SD card

## Build Configuration

### PlatformIO Configuration

```ini
; platformio.ini
[common]
build_flags = 
    -I ${PROJECT_DIR}/src
    -I ${PROJECT_DIR}/src/include

[env:arduino_nano_esp32]
platform = espressif32
board = arduino_nano_esp32
framework = arduino
build_flags = 
    ${common.build_flags}
    -DPLATFORM_ESP32
    -std=gnu++17
lib_deps = 
    FastLED
    arduino-libraries/ArduinoBLE@^1.4.0
    adafruit/Adafruit SSD1306@^2.5.7
    adafruit/Adafruit GFX Library@^1.11.5
    adafruit/Adafruit BusIO@^1.14.1

[env:raspberry-pi-pico]
platform = raspberrypi
board = raspberry-pi-pico
framework = arduino
build_flags = 
    ${common.build_flags}
    -DPLATFORM_RP2040
    -std=gnu++17
lib_deps = 
    FastLED
    arduino-pico/BLE@^1.0.0
    adafruit/Adafruit SSD1306@^2.5.7
    adafruit/Adafruit GFX Library@^1.11.5
    adafruit/Adafruit BusIO@^1.14.1
```

## Implementation Details

### Core Logic Preservation

The main application logic remains unchanged:

```cpp
// main.cpp - mostly unchanged
int main() {
    // Platform-specific initialization
    auto ledController = PlatformFactory::createLEDController();
    auto bleController = PlatformFactory::createBLEController();
    
    // Core logic unchanged!
    bleManager.begin();
    patternManager.begin();
    displayTask.begin();
}
```

### HAL Directory Structure

```
src/
├── hal/
│   ├── LEDController.h
│   ├── ESP32LEDController.h
│   ├── ESP32LEDController.cpp
│   ├── RP2040LEDController.h
│   ├── RP2040LEDController.cpp
│   ├── BLEController.h
│   ├── ESP32BLEController.h
│   ├── ESP32BLEController.cpp
│   ├── RP2040BLEController.h
│   ├── RP2040BLEController.cpp
│   ├── DisplayController.h
│   ├── ESP32DisplayController.h
│   ├── ESP32DisplayController.cpp
│   ├── RP2040DisplayController.h
│   ├── RP2040DisplayController.cpp
│   ├── SystemController.h
│   ├── ESP32SystemController.h
│   ├── ESP32SystemController.cpp
│   ├── RP2040SystemController.h
│   ├── RP2040SystemController.cpp
│   └── PlatformFactory.h
```

## Development Workflow

### Branch Strategy
- **main**: Current stable ESP32-S3 version
- **feature/rp2040-support**: Development branch for RP2040 support
- **feature/hal-refactor**: HAL abstraction layer development

### Testing Strategy
1. **Phase testing**: Test each phase independently
2. **Regression testing**: Ensure ESP32-S3 functionality remains intact
3. **Integration testing**: Test full feature set on both platforms
4. **Performance testing**: Compare performance between platforms

### Migration Path
1. **Create HAL interfaces** alongside existing code
2. **Implement ESP32 versions** first (proven functionality)
3. **Implement RP2040 versions** with simplified features
4. **Gradually migrate** core code to use HAL
5. **Remove platform-specific code** once HAL is complete

## Future Considerations

### Additional Platforms
- **ESP32-C3**: Single-core, lower cost alternative
- **STM32**: ARM Cortex-M4/M7 options
- **nRF52**: Nordic BLE-focused microcontrollers

### Feature Parity
- **WiFi support**: External modules for non-ESP32 platforms
- **Audio processing**: RP2040's built-in microphone advantage
- **Performance optimization**: Platform-specific optimizations

## Conclusion

This incremental HAL approach allows SRDriver to support multiple platforms while maintaining code quality and reducing development risk. The phased implementation strategy ensures that each step is tested and validated before moving to the next phase.

The key benefits are:
- **Low risk**: Test each phase independently
- **Maintainable**: Core logic doesn't change
- **Scalable**: Easy to add more platforms later
- **Proven pattern**: Based on successful industry practices
