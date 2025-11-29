#pragma once

#include <Arduino.h>
#include <functional>
#include <map>
#include <vector>
#include "InputEvent.h"

using InputEventCallback = std::function<void(const InputEvent&)>;

/**
 * Registry for managing input event callbacks
 * Supports registration for specific devices, specific events, or globally
 */
class InputCallbackRegistry {
private:
    // Callbacks indexed by device name and event type
    std::map<String, std::map<InputEventType, std::vector<InputEventCallback>>> callbacks;
    
    // Global callbacks (for all devices/events)
    std::vector<InputEventCallback> globalCallbacks;
    
public:
    /**
     * Register callback for specific device and event
     * @param deviceName Name of the device
     * @param eventType Type of event to listen for
     * @param callback Function to call when event occurs
     */
    void registerCallback(const String& deviceName, InputEventType eventType, InputEventCallback callback) {
        callbacks[deviceName][eventType].push_back(callback);
    }
    
    /**
     * Register callback for all events from a specific device
     * @param deviceName Name of the device
     * @param callback Function to call when any event occurs from this device
     */
    void registerDeviceCallback(const String& deviceName, InputEventCallback callback) {
        for (auto eventType : {InputEventType::BUTTON_PRESS, InputEventType::BUTTON_HOLD, 
                              InputEventType::BUTTON_RELEASE, InputEventType::POTENTIOMETER_CHANGE,
                              InputEventType::MICROPHONE_AUDIO_DETECTED, InputEventType::MICROPHONE_CLIPPING,
                              InputEventType::GENERIC_VALUE_CHANGE}) {
            callbacks[deviceName][eventType].push_back(callback);
        }
    }
    
    /**
     * Register global callback for all events from all devices
     * @param callback Function to call when any event occurs
     */
    void registerGlobalCallback(InputEventCallback callback) {
        globalCallbacks.push_back(callback);
    }
    
    /**
     * Trigger callbacks for an event
     * @param event The event to trigger callbacks for
     */
    void triggerCallbacks(const InputEvent& event) {
        // Trigger global callbacks
        for (auto& callback : globalCallbacks) {
            callback(event);
        }
        
        // Trigger device-specific callbacks
        auto deviceIt = callbacks.find(event.deviceName);
        if (deviceIt != callbacks.end()) {
            auto eventIt = deviceIt->second.find(event.eventType);
            if (eventIt != deviceIt->second.end()) {
                for (auto& callback : eventIt->second) {
                    callback(event);
                }
            }
        }
    }
    
    /**
     * Clear all registered callbacks
     */
    void clear() {
        callbacks.clear();
        globalCallbacks.clear();
    }
    
    /**
     * Get number of registered callbacks for debugging
     * @return Total number of registered callbacks
     */
    size_t getCallbackCount() const {
        size_t count = globalCallbacks.size();
        for (const auto& device : callbacks) {
            for (const auto& event : device.second) {
                count += event.second.size();
            }
        }
        return count;
    }
}; 