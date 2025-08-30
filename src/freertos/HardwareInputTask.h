#pragma once

#include "../freertos/SRTask.h"
#include "../hal/input/InputDeviceRegistry.h"
#include "../hal/input/InputCallbackRegistry.h"
#include "../hal/input/InputEvent.h"
#include "../hal/input/InputDeviceConfig.h"
#include "../hal/input/buttons/ButtonInputDevice.h"
#include "../hal/input/potentiometers/PotentiometerInputDevice.h"
#include "../hal/input/audio/MicrophoneInputDevice.h"
#include "freertos/queue.h"

/**
 * FreeRTOS task for polling hardware inputs
 * Manages device registry, callback registry, and event generation
 */
class HardwareInputTask : public SRTask {
private:
    InputDeviceRegistry deviceRegistry;
    InputCallbackRegistry callbackRegistry;
    std::vector<InputDeviceConfig> deviceConfigs;
    
    // Event queue for other tasks
    QueueHandle_t inputEventQueue;
    
    // Polling interval
    uint32_t baseIntervalMs;
    
public:
    /**
     * Constructor
     * @param configs Vector of device configurations
     * @param baseIntervalMs Base polling interval in milliseconds
     */
    HardwareInputTask(const std::vector<InputDeviceConfig>& configs, uint32_t baseIntervalMs = 50)
        : SRTask("HardwareInput", 4096, tskIDLE_PRIORITY + 2, 0),
          deviceConfigs(configs), baseIntervalMs(baseIntervalMs) {
        initializeDevices();
        inputEventQueue = xQueueCreate(20, sizeof(InputEvent));
    }
    
    /**
     * Destructor
     */
    ~HardwareInputTask() {
        if (inputEventQueue) {
            vQueueDelete(inputEventQueue);
        }
    }
    
    // Generic callback registration methods
    void registerCallback(const String& deviceName, InputEventType eventType, InputEventCallback callback) {
        callbackRegistry.registerCallback(deviceName, eventType, callback);
    }
    
    void registerDeviceCallback(const String& deviceName, InputEventCallback callback) {
        callbackRegistry.registerDeviceCallback(deviceName, callback);
    }
    
    void registerGlobalCallback(InputEventCallback callback) {
        callbackRegistry.registerGlobalCallback(callback);
    }
    
    // Queue access for other tasks
    QueueHandle_t getInputEventQueue() const { return inputEventQueue; }
    
    // Device access methods
    InputDevice* getDevice(const String& name) {
        return deviceRegistry.getDevice(name);
    }
    
    const InputDevice* getDevice(const String& name) const {
        return deviceRegistry.getDevice(name);
    }
    
    std::vector<String> getDeviceNames() const {
        return deviceRegistry.getDeviceNames();
    }
    
    size_t getDeviceCount() const {
        return deviceRegistry.getDeviceCount();
    }
    
    size_t getCallbackCount() const {
        return callbackRegistry.getCallbackCount();
    }

    InputCallbackRegistry& getCallbackRegistry() {
        return callbackRegistry;
    }
    
protected:
    /**
     * Main task function
     */
    virtual void run() override {
        TickType_t lastWakeTime = xTaskGetTickCount();
        
        while (true) {
            // Poll all devices
            deviceRegistry.pollAll();
            
            // Check for changes and trigger callbacks
            checkForChanges();
            
            // Sleep until next poll cycle
            vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(baseIntervalMs));
        }
    }
    
private:
    /**
     * Initialize devices from configuration
     */
    void initializeDevices() {
        for (const auto& config : deviceConfigs) {
            if (config.type == "button") {
                deviceRegistry.registerDevice<ButtonInputDevice>(config.name, config.name, config.pin);
            }
            else if (config.type == "potentiometer") {
                deviceRegistry.registerDevice<PotentiometerInputDevice>(config.name, config.name, config.pin);
                
                // Set hysteresis threshold if specified
                auto device = deviceRegistry.getDevice(config.name);
                if (device) {
                    auto potDevice = static_cast<PotentiometerInputDevice*>(device);
                    potDevice->setHysteresisThreshold(config.hysteresisThreshold);
                }
            }
            else if (config.type == "microphone") {
                deviceRegistry.registerDevice<MicrophoneInputDevice>(config.name, config.name, config.pin, 
                                                                    config.sampleRate, config.sampleWindow);
            }
        }
    }
    
    /**
     * Check for device changes and generate events
     */
    void checkForChanges() {
        for (const auto& deviceName : deviceRegistry.getDeviceNames()) {
            auto device = deviceRegistry.getDevice(deviceName);
            if (device && device->hasChanged()) {
                InputEvent event = createEventFromDevice(device);
                
                // Trigger callbacks
                callbackRegistry.triggerCallbacks(event);
                
                // Send to queue for other tasks
                if (inputEventQueue) {
                    xQueueSend(inputEventQueue, &event, 0);
                }
                
                device->resetChanged();
            }
        }
    }
    
    /**
     * Create InputEvent from device state
     * @param device Pointer to input device
     * @return InputEvent structure
     */
    InputEvent createEventFromDevice(InputDevice* device) {
        InputEvent event;
        event.deviceName = device->getName();
        event.timestamp = millis();
        
        // Device-specific event creation
        if (device->getDeviceType() == "button") {
            auto buttonDevice = static_cast<ButtonInputDevice*>(device);
            auto buttonEvent = buttonDevice->getCurrentEvent();
            
            switch (buttonEvent) {
                case ButtonEvent::PRESS:
                    event.eventType = InputEventType::BUTTON_PRESS;
                    break;
                case ButtonEvent::HOLD:
                    event.eventType = InputEventType::BUTTON_HOLD;
                    break;
                default:
                    event.eventType = InputEventType::BUTTON_RELEASE;
                    break;
            }
            event.value = static_cast<int>(buttonEvent);
            event.mappedValue = event.value;
        }
        else if (device->getDeviceType() == "potentiometer") {
            auto potDevice = static_cast<PotentiometerInputDevice*>(device);
            event.eventType = InputEventType::POTENTIOMETER_CHANGE;
            event.value = potDevice->getRawValue();
            event.mappedValue = potDevice->getMappedValue(0, 255);
        }
        else if (device->getDeviceType() == "microphone") {
            auto micDevice = static_cast<MicrophoneInputDevice*>(device);
            event.eventType = InputEventType::GENERIC_VALUE_CHANGE;
            event.value = micDevice->getAudioLevel();
            event.mappedValue = micDevice->getVolumeDB();
            
            // Check for specific microphone events
            if (micDevice->isAudioDetected()) {
                event.eventType = InputEventType::MICROPHONE_AUDIO_DETECTED;
            }
            if (micDevice->isClipping()) {
                event.eventType = InputEventType::MICROPHONE_CLIPPING;
            }
        }
        
        return event;
    }
}; 