#pragma once

#include <Arduino.h>
#include <vector>
#include <memory>
#include <map>
#include "config/JsonSettings.h"

#if !defined(D0)
    #define D0 0
    #define D1 1
    #define D2 2
    #define D3 3
    #define D4 4
    #define D5 5
    #define D6 6
    #define D7 7
    #define D8 8
    #define D9 9
    #define D10 10
    #define D11 11
    #define D12 12
    #define D13 13
    #define A0 14
    #define A1 15
    #define A2 16
    #define A3 17
    #define A4 18
    #define A5 19
    #define A6 20
    #define A7 21
#endif

static const std::map<String, int> pinMap = {
    {"D0", D0}, {"D1", D1}, {"D2", D2}, {"D3", D3}, {"D4", D4},
    {"D5", D5}, {"D6", D6}, {"D7", D7}, {"D8", D8}, {"D9", D9}, {"D10", D10},
    {"D11", D11}, {"D12", D12}, {"D13", D13},
    {"A0", A0}, {"A1", A1}, {"A2", A2}, {"A3", A3}, {"A4", A4}, {"A5", A5},
};

inline int pinNameToNumber(const String& pinName) {
    // Allow setting pin: "GPIO1" or whatever, but this integer maps
    // directly, not using the macro helpers
    if (pinName.startsWith("GPIO"))
    {
        return pinName.substring(4).toInt();
    }
    auto it = pinMap.find(pinName);
    if (it != pinMap.end()) {
        return it->second;
    }
    return pinName.toInt();
}

/**
 * Configuration structure for input devices
 */
struct InputDeviceConfig {
    String name;
    String type;
    int pin;
    uint32_t pollIntervalMs;
    
    // Additional parameters for specific device types
    int sampleRate;      // For microphones
    int sampleWindow;    // For microphones
    int hysteresisThreshold; // For potentiometers
    int minDiff = 0;
    int bumpLimit = 0;
    int bitShift = 0;
    
    InputDeviceConfig() : pin(-1), pollIntervalMs(50), sampleRate(8000), 
                         sampleWindow(50), hysteresisThreshold(50) {}
    
    InputDeviceConfig(const String& deviceName, const String& deviceType, int devicePin, uint32_t interval = 50)
        : name(deviceName), type(deviceType), pin(devicePin), pollIntervalMs(interval),
          sampleRate(8000), sampleWindow(50), hysteresisThreshold(50) {}
};

/**
 * Builder class for creating HardwareInputTask instances
 * Supports both manual configuration and JSON loading
 */
class HardwareInputTaskBuilder {
private:
    std::vector<InputDeviceConfig> configs;
    
public:
    /**
     * Add a button device
     * @param name Device name
     * @param pin GPIO pin number
     * @param pollIntervalMs Polling interval in milliseconds
     * @return Reference to builder for chaining
     */
    HardwareInputTaskBuilder& addButton(const String& name, int pin, uint32_t pollIntervalMs = 50) {
        configs.push_back(InputDeviceConfig(name, "button", pin, pollIntervalMs));
        return *this;
    }
    
    /**
     * Add a potentiometer device
     * @param name Device name
     * @param pin Analog pin number
     * @param pollIntervalMs Polling interval in milliseconds
     * @param hysteresisThreshold Hysteresis threshold
     * @return Reference to builder for chaining
     */
    HardwareInputTaskBuilder& addPotentiometer(const String& name, int pin, uint32_t pollIntervalMs = 100, int hysteresisThreshold = 50) {
        InputDeviceConfig config(name, "potentiometer", pin, pollIntervalMs);
        config.hysteresisThreshold = hysteresisThreshold;
        configs.push_back(config);
        return *this;
    }
    
    /**
     * Add a slide potentiometer device using the slidePot class
     * @param name Device name
     * @param pin Analog pin number
     * @param pollIntervalMs Polling interval in milliseconds
     * @param bitShift Bit shift for noise reduction (default: 3)
     * @param minDiff Minimum difference threshold (default: 1)
     * @param bumpLimit Number of consecutive readings required (default: 3)
     * @return Reference to builder for chaining
     */
    HardwareInputTaskBuilder& addSlidePotentiometer(const String& name, int pin, uint32_t pollIntervalMs = 100, 
                                                   int bitShift = 3, int minDiff = 1, int bumpLimit = 3) {
        InputDeviceConfig config(name, "slide_potentiometer", pin, pollIntervalMs);
        config.hysteresisThreshold = minDiff;  // Use minDiff as hysteresis
        config.bitShift = bitShift;
        config.minDiff = minDiff;
        config.bumpLimit = bumpLimit;
        configs.push_back(config);
        return *this;
    }
    
    /**
     * Add a microphone device
     * @param name Device name
     * @param pin Analog pin number
     * @param pollIntervalMs Polling interval in milliseconds
     * @param sampleRate Sample rate in Hz
     * @param sampleWindow Sample window in milliseconds
     * @return Reference to builder for chaining
     */
    HardwareInputTaskBuilder& addMicrophone(const String& name, int pin, uint32_t pollIntervalMs = 20, 
                                           int sampleRate = 8000, int sampleWindow = 50) {
        InputDeviceConfig config(name, "microphone", pin, pollIntervalMs);
        config.sampleRate = sampleRate;
        config.sampleWindow = sampleWindow;
        configs.push_back(config);
        return *this;
    }
    
    /**
     * Load configuration from JSON document
     * @param json DynamicJsonDocument containing device configurations
     * @return Reference to builder for chaining
     */
    HardwareInputTaskBuilder& fromJson(const String& jsonFileName); // Implementation in .cpp file
    
    /**
     * Load configuration from JSON file
     * @param filename Path to JSON configuration file
     * @return Reference to builder for chaining
     */
    HardwareInputTaskBuilder& fromJsonFile(const String& filename) {
        // This would need to be implemented based on your file system
        // For now, we'll just return *this
        // TODO: Implement file loading
        return *this;
    }
    
    /**
     * Check if configuration is valid
     * @return true if valid, false otherwise
     */
    bool isValid() const {
        return !configs.empty();
    }
    
    /**
     * Get current configurations (for debugging)
     * @return Vector of device configurations
     */
    const std::vector<InputDeviceConfig>& getConfigs() const {
        return configs;
    }
    
    /**
     * Get number of configured devices
     * @return Number of devices
     */
    size_t getDeviceCount() const {
        return configs.size();
    }
    
    /**
     * Clear all configurations
     * @return Reference to builder for chaining
     */
    HardwareInputTaskBuilder& clear() {
        configs.clear();
        return *this;
    }
    
    /**
     * Build HardwareInputTask with raw pointer (for compatibility)
     * @return Pointer to HardwareInputTask, or nullptr if invalid
     */
    class HardwareInputTask* build();
    
    /**
     * Build HardwareInputTask with unique_ptr (for future use)
     * @return Unique pointer to HardwareInputTask, or nullptr if invalid
     */
    std::unique_ptr<class HardwareInputTask> buildUnique();
}; 