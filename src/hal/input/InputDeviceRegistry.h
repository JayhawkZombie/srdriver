#pragma once

#include <Arduino.h>
#include <map>
#include <memory>
#include <vector>
#include "InputDevice.h"

/**
 * Registry for managing input devices by name
 * Provides a unified interface for device management and polling
 */
class InputDeviceRegistry {
private:
    std::map<String, std::unique_ptr<InputDevice>> devices;
    
public:
    /**
     * Register a new input device
     * @param name Unique name for the device
     * @param device Unique pointer to the device
     */
    template<typename T, typename... Args>
    void registerDevice(const String& name, Args&&... args) {
        devices[name] = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
    
    /**
     * Get a device by name
     * @param name Device name
     * @return Pointer to device, or nullptr if not found
     */
    InputDevice* getDevice(const String& name) {
        auto it = devices.find(name);
        return (it != devices.end()) ? it->second.get() : nullptr;
    }
    
    /**
     * Get a device by name (const version)
     * @param name Device name
     * @return Pointer to device, or nullptr if not found
     */
    const InputDevice* getDevice(const String& name) const {
        auto it = devices.find(name);
        return (it != devices.end()) ? it->second.get() : nullptr;
    }
    
    /**
     * Get all device names
     * @return Vector of device names
     */
    std::vector<String> getDeviceNames() const {
        std::vector<String> names;
        for (const auto& pair : devices) {
            names.push_back(pair.first);
        }
        return names;
    }
    
    /**
     * Poll all registered devices
     */
    void pollAll() {
        for (auto& pair : devices) {
            pair.second->poll();
        }
    }
    
    /**
     * Check if any device has changed
     * @return true if any device has changed, false otherwise
     */
    bool hasAnyChanged() const {
        for (const auto& pair : devices) {
            if (pair.second->hasChanged()) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * Get number of registered devices
     * @return Number of devices
     */
    size_t getDeviceCount() const {
        return devices.size();
    }
    
    /**
     * Remove a device by name
     * @param name Device name to remove
     * @return true if device was removed, false if not found
     */
    bool removeDevice(const String& name) {
        auto it = devices.find(name);
        if (it != devices.end()) {
            devices.erase(it);
            return true;
        }
        return false;
    }
    
    /**
     * Clear all devices
     */
    void clear() {
        devices.clear();
    }
    
    /**
     * Check if device exists
     * @param name Device name
     * @return true if device exists, false otherwise
     */
    bool hasDevice(const String& name) const {
        return devices.find(name) != devices.end();
    }
}; 