# SRDriver Platform Abstractions

## Overview

SRDriver uses compile-time platform abstractions to support different hardware configurations while maintaining clean, maintainable code. This approach builds on existing patterns like RGB vs RGBW LED detection and extends them to broader platform support.

## Existing Compile-Time Abstractions

### RGB vs RGBW LED Detection

SRDriver already implements compile-time LED type detection:

```cpp
// main.cpp
#define FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED 0

// PatternManager.cpp
#if FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED
    // RGBW-specific code path
    FastLED.addLeds<WS2812B, LED_PIN, GRBW>(leds, NUM_LEDS);
#else
    // Standard RGB code path
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
#endif
```

**Benefits**:
- **Single codebase** for both RGB and RGBW variants
- **Compile-time optimization** - no runtime overhead
- **Type safety** - compiler catches configuration mismatches
- **Easy testing** - build both variants from same source

## Extended Platform Abstraction Strategy

### HAL (Hardware Abstraction Layer) Design

Building on the RGB/RGBW pattern, we extend to full platform abstraction:

```cpp
// hal/LEDController.h
class LEDController {
public:
    virtual void begin() = 0;
    virtual void setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w) = 0;
    virtual void show() = 0;
    virtual void clear() = 0;
    
    // Platform-specific features
    virtual bool supportsRGBW() const = 0;
    virtual uint16_t getMaxBrightness() const = 0;
};
```

### Platform Factory Pattern

```cpp
// hal/PlatformFactory.h
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
    
    static DisplayController* createDisplayController() {
#ifdef PLATFORM_ESP32
        return new ESP32DisplayController();
#elif defined(PLATFORM_RP2040)
        return new RP2040DisplayController();
#endif
    }
};
```

## Abstraction Benefits Beyond Multi-Platform

### 1. **Custom LED Protocols**

Even on the same platform, different LED types require different handling:

```cpp
// hal/ESP32LEDController.h
class ESP32LEDController : public LEDController {
private:
    LEDType _ledType;
    
public:
    ESP32LEDController(LEDType type) : _ledType(type) {}
    
    void begin() override {
        switch (_ledType) {
            case LEDType::WS2812B_RGB:
                FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
                break;
            case LEDType::WS2812B_RGBW:
                FastLED.addLeds<WS2812B, LED_PIN, GRBW>(leds, NUM_LEDS);
                break;
            case LEDType::APA102:
                FastLED.addLeds<APA102, LED_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS);
                break;
            case LEDType::SK6812:
                FastLED.addLeds<SK6812, LED_PIN, GRB>(leds, NUM_LEDS);
                break;
        }
    }
    
    bool supportsRGBW() const override {
        return _ledType == LEDType::WS2812B_RGBW;
    }
};
```

### 2. **Power Management**

Different platforms have different power constraints:

```cpp
// hal/PowerManagedLEDController.h
class PowerManagedLEDController : public LEDController {
private:
    LEDController* _baseController;
    uint32_t _maxPowerMilliwatts;
    uint32_t _currentPower;
    
public:
    void setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) override {
        uint32_t pixelPower = calculatePower(r, g, b);
        
        if (_currentPower + pixelPower > _maxPowerMilliwatts) {
            // Scale down brightness to stay within power budget
            float scale = (float)(_maxPowerMilliwatts - _currentPower) / pixelPower;
            r = (uint8_t)(r * scale);
            g = (uint8_t)(g * scale);
            b = (uint8_t)(b * scale);
        }
        
        _baseController->setPixel(index, r, g, b);
        _currentPower += calculatePower(r, g, b);
    }
};
```

### 3. **Custom Timing Requirements**

Some LED protocols require precise timing:

```cpp
// hal/PrecisionLEDController.h
class PrecisionLEDController : public LEDController {
private:
    LEDController* _baseController;
    
public:
    void show() override {
        // Disable interrupts for precise timing
        noInterrupts();
        _baseController->show();
        interrupts();
    }
};
```

### 4. **Testing and Mocking**

Abstractions enable easy testing:

```cpp
// hal/MockLEDController.h
class MockLEDController : public LEDController {
private:
    std::vector<CRGB> _pixels;
    
public:
    void setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) override {
        if (index < _pixels.size()) {
            _pixels[index] = CRGB(r, g, b);
        }
    }
    
    CRGB getPixel(uint16_t index) const {
        return (index < _pixels.size()) ? _pixels[index] : CRGB(0, 0, 0);
    }
};
```

## Implementation Strategy

### Phase 1: Extend Existing RGB/RGBW Pattern

```cpp
// hal/LEDType.h
enum class LEDType {
    WS2812B_RGB,
    WS2812B_RGBW,
    APA102,
    SK6812,
    CUSTOM
};

// hal/LEDController.h
class LEDController {
public:
    virtual void begin() = 0;
    virtual void setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w) = 0;
    virtual void show() = 0;
    virtual void clear() = 0;
    virtual bool supportsRGBW() const = 0;
};
```

### Phase 2: Platform-Specific Implementations

```cpp
// hal/ESP32LEDController.h
class ESP32LEDController : public LEDController {
private:
    LEDType _ledType;
    
public:
    ESP32LEDController(LEDType type) : _ledType(type) {}
    
    void begin() override {
        switch (_ledType) {
            case LEDType::WS2812B_RGB:
                FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
                break;
            case LEDType::WS2812B_RGBW:
                FastLED.addLeds<WS2812B, LED_PIN, GRBW>(leds, NUM_LEDS);
                break;
        }
    }
    
    bool supportsRGBW() const override {
        return _ledType == LEDType::WS2812B_RGBW;
    }
};
```

### Phase 3: Factory Pattern Integration

```cpp
// hal/PlatformFactory.h
class PlatformFactory {
public:
    static LEDController* createLEDController(LEDType type = LEDType::WS2812B_RGB) {
#ifdef PLATFORM_ESP32
        return new ESP32LEDController(type);
#elif defined(PLATFORM_RP2040)
        return new RP2040LEDController(type);
#endif
    }
};
```

## Build Configuration

### PlatformIO Configuration

```ini
; platformio.ini
[common]
build_flags = 
    -I ${PROJECT_DIR}/src
    -I ${PROJECT_DIR}/src/include

[env:arduino_nano_esp32_rgb]
platform = espressif32
board = arduino_nano_esp32
framework = arduino
build_flags = 
    ${common.build_flags}
    -DPLATFORM_ESP32
    -DLED_TYPE=WS2812B_RGB
    -std=gnu++17

[env:arduino_nano_esp32_rgbw]
platform = espressif32
board = arduino_nano_esp32
framework = arduino
build_flags = 
    ${common.build_flags}
    -DPLATFORM_ESP32
    -DLED_TYPE=WS2812B_RGBW
    -std=gnu++17

[env:raspberry-pi-pico]
platform = raspberrypi
board = raspberry-pi-pico
framework = arduino
build_flags = 
    ${common.build_flags}
    -DPLATFORM_RP2040
    -DLED_TYPE=WS2812B_RGB
    -std=gnu++17
```

## Benefits of This Approach

### 1. **Natural Evolution**
- Builds on existing RGB/RGBW pattern
- Familiar compile-time configuration
- Gradual migration path

### 2. **Flexibility**
- Support any LED type with custom protocols
- Platform-specific optimizations
- Power management integration

### 3. **Testability**
- Mock implementations for unit testing
- Hardware-in-the-loop testing
- Automated testing on multiple configurations

### 4. **Maintainability**
- Single codebase for all variants
- Clear separation of concerns
- Easy to add new LED types or platforms

### 5. **Performance**
- Compile-time optimization
- No runtime overhead for abstractions
- Platform-specific optimizations

## Future Extensions

### 1. **Audio Integration**
```cpp
class AudioLEDController : public LEDController {
    // Synchronize LED patterns with audio input
    // Use RP2040's built-in microphone
};
```

### 2. **Network Integration**
```cpp
class NetworkLEDController : public LEDController {
    // Remote LED control over WiFi/BLE
    // Multi-device synchronization
};
```

### 3. **Sensor Integration**
```cpp
class SensorLEDController : public LEDController {
    // LED patterns based on environmental sensors
    // Temperature, humidity, light level
};
```

## Initial Implementation Phase

### Phase 1: LED Controller Only (Minimal Refactor)

This phase implements only the LED controller abstraction, keeping all other features unchanged. This allows for easy LTS merges and minimal risk.

#### **Clean PlatformFactory Implementation**

```cpp
// hal/PlatformFactory.h
class PlatformFactory {
public:
    // LED Controller - the only abstraction in Phase 1
    static LEDController* createLEDController() {
#ifdef PLATFORM_ESP32
        return new ESP32LEDController();
#elif defined(PLATFORM_RP2040)
        return new RP2040LEDController();
#else
        #error "Unsupported platform"
#endif
    }
    
    // Feature availability checks
    static bool supportsBLE() {
#ifdef PLATFORM_ESP32
        return true;
#elif defined(PLATFORM_RP2040)
        return false;  // Disabled in Phase 1
#endif
    }
    
    static bool supportsDisplay() {
#ifdef PLATFORM_ESP32
        return true;
#elif defined(PLATFORM_RP2040)
        return false;  // Disabled in Phase 1
#endif
    }
    
    static bool supportsWiFi() {
#ifdef PLATFORM_ESP32
        return true;
#elif defined(PLATFORM_RP2040)
        return false;  // No WiFi on RP2040
#endif
    }
};
```

#### **Thin Task Refactoring**

Minimal changes to existing tasks to use the PlatformFactory:

```cpp
// freertos/LEDUpdateTask.h - minimal changes
class LEDUpdateTask : public SRTask {
private:
    LEDController* _ledController;  // New: injected dependency
    
public:
    LEDUpdateTask(uint32_t interval, LEDController* ledController) 
        : SRTask("LEDUpdate", 4096, tskIDLE_PRIORITY + 2, 1),
          _interval(interval),
          _ledController(ledController) {}  // New: accept controller
    
    void run() override {
        while (_running) {
            // Existing pattern logic unchanged
            updatePatterns();
            
            // Only change: use injected controller
            _ledController->show();  // Instead of FastLED.show()
            
            sleep(_interval);
        }
    }
};
```

#### **Main.cpp Integration**

```cpp
// main.cpp - minimal changes
int main() {
    // Platform-specific initialization
    auto ledController = PlatformFactory::createLEDController();
    ledController->begin();
    
    // Initialize tasks with feature checks
    if (PlatformFactory::supportsBLE()) {
        // Initialize BLE (ESP32 only)
        bleManager.begin();
        g_bleUpdateTask = new BLEUpdateTask(bleManager);
    }
    
    if (PlatformFactory::supportsDisplay()) {
        // Initialize display (ESP32 only)
        g_displayTask = new DisplayTask(33);
    }
    
    // LED task works on all platforms
    g_ledUpdateTask = new LEDUpdateTask(16, ledController);
    
    // Rest of initialization unchanged
    // ...
}
```

#### **RP2040 Limitations in Phase 1**

```cpp
// hal/RP2040LEDController.h
class RP2040LEDController : public LEDController {
public:
    void begin() override {
        // Same FastLED setup as ESP32
        FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
        FastLED.setBrightness(128);
    }
    
    void setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) override {
        FastLED.leds[index] = CRGB(r, g, b);
    }
    
    void show() override {
        FastLED.show();
    }
    
    bool supportsRGBW() const override {
        return false;  // RP2040 Phase 1: RGB only
    }
};
```

#### **Build Configuration for Phase 1**

```ini
; platformio.ini
[env:arduino_nano_esp32]
platform = espressif32
board = arduino_nano_esp32
framework = arduino
build_flags = 
    -DPLATFORM_ESP32
    -std=gnu++17
lib_deps = 
    FastLED
    arduino-libraries/ArduinoBLE@^1.4.0
    adafruit/Adafruit SSD1306@^2.5.7
    # All existing dependencies

[env:raspberry-pi-pico]
platform = raspberrypi
board = raspberry-pi-pico
framework = arduino
build_flags = 
    -DPLATFORM_RP2040
    -std=gnu++17
lib_deps = 
    FastLED
    # Only LED-related dependencies
    # No BLE, display, or WiFi libraries
```

### **Benefits of Phase 1 Approach**

1. **Minimal Risk**: Only LED controller changes, everything else unchanged
2. **Easy LTS Merges**: Core logic remains the same, just dependency injection
3. **Proven Pattern**: Builds on existing RGB/RGBW compile-time checks
4. **Clean Separation**: Platform-specific code isolated in HAL layer
5. **Gradual Migration**: Can add more abstractions in future phases

### **LTS Merge Strategy**

```cpp
// Feature flags for easy LTS merges
#ifdef PLATFORM_ESP32
    #define ENABLE_BLE 1
    #define ENABLE_DISPLAY 1
    #define ENABLE_WIFI 1
#elif defined(PLATFORM_RP2040)
    #define ENABLE_BLE 0
    #define ENABLE_DISPLAY 0
    #define ENABLE_WIFI 0
#endif

// In main.cpp
#if ENABLE_BLE
    bleManager.begin();
#endif

#if ENABLE_DISPLAY
    g_displayTask = new DisplayTask(33);
#endif
```

This approach ensures that LTS merges are straightforward - the core logic remains unchanged, and platform-specific features are simply disabled via compile-time flags.

## Conclusion

The platform abstraction approach builds naturally on SRDriver's existing compile-time configuration patterns. By extending the RGB/RGBW detection to full platform abstraction, we gain:

- **Multi-platform support** without code duplication
- **Custom LED protocol support** for specialized hardware
- **Power management** for battery-powered applications
- **Testing capabilities** for reliable development
- **Future extensibility** for new features and platforms

This approach maintains the performance and reliability of compile-time configuration while providing the flexibility needed for diverse hardware configurations.
