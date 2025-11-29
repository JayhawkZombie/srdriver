#include "controllers/BrightnessController.h"
#include "hal/ble/BLEManager.h"
#include "freertos/LogManager.h"
#include "Utils.hpp"
#include <FastLED.h>

// Singleton instance
BrightnessController* BrightnessController::instance = nullptr;

BrightnessController::BrightnessController() 
    : currentBrightness(128), targetBrightness(128), isPulsing(false), 
      pulseStartTime(0), pulseDuration(0), isFadeMode(false) {
    
    // Initialize BLE characteristic info
    brightnessCharacteristicInfo.characteristicUuid = "4df3a1f9-2a42-43ee-ac96-f7db09abb4f0";
    brightnessCharacteristicInfo.descriptorUuid = "2901";
    brightnessCharacteristicInfo.formatDescriptorUuid = "2904";
    brightnessCharacteristicInfo.name = "Brightness Control";
    brightnessCharacteristicInfo.description = "Controls LED brightness (0-255)";
    brightnessCharacteristicInfo.isWritable = true;
    brightnessCharacteristicInfo.isReadable = true;
    brightnessCharacteristicInfo.isNotifiable = true;
    brightnessCharacteristicInfo.maxValueLength = 3;
    
    // Format data for 2904 descriptor
    brightnessCharacteristicInfo.formatData = {
        0x1A,      // UTF-8 String
        0,         // No exponent
        0x0000,    // No unit
        0x01,      // Bluetooth SIG namespace
        0x0000     // No description
    };
    
    // Set up BLE callbacks
    brightnessCharacteristicInfo.onWrite = [this](const unsigned char* value, size_t length) {
        // Handle brightness write from BLE
        char buf[16];
        size_t len = std::min(sizeof(buf) - 1, length);
        memcpy(buf, value, len);
        buf[len] = '\0';
        String s(buf);
        int rawVal = s.toInt();
        
        // Apply the same mapping logic as the original handler
        float mapped = getVaryingCurveMappedValue(rawVal / 255.0f, 3.f);
        int mappedVal = static_cast<int>(mapped * 255.0f + 0.5f);
        
        // Update internal state
        setBrightness(mappedVal);
        
        // Update the characteristic with the mapped value
        if (brightnessCharacteristicInfo.characteristic) {
            brightnessCharacteristicInfo.characteristic->writeValue(String(mappedVal).c_str());
        }
    };
    
    brightnessCharacteristicInfo.onRead = [this]() -> String {
        return String(currentBrightness);
    };
}

void BrightnessController::initialize() {
    LOG_DEBUG_COMPONENT("Brightness", "initialize() called");
    if (instance == nullptr) {
        LOG_DEBUG_COMPONENT("Brightness", "Creating new instance...");
        instance = new BrightnessController();
        LOG_DEBUG_COMPONENT("Brightness", "Controller initialized");
    } else {
        LOG_DEBUG_COMPONENT("Brightness", "Instance already exists");
    }
}

void BrightnessController::destroy() {
    LOG_DEBUG_COMPONENT("Brightness", "destroy() called");
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
        LOG_DEBUG_COMPONENT("Brightness", "Controller destroyed");
    }
}

BrightnessController* BrightnessController::getInstance() {
    // LOG_DEBUG_COMPONENT("Brightness", "getInstance() called, returning: %s", instance ? "valid pointer" : "nullptr");
    return instance;
}

void BrightnessController::updateBrightness(int brightness) {
    LOG_DEBUGF_COMPONENT("Brightness", "Updating brightness to %d", brightness);
    float mapped = getVaryingCurveMappedValue(brightness / 255.0f, 3.f);
    int mappedVal = static_cast<int>(mapped * 255.0f + 0.5f);
    setBrightness(mappedVal);
}

void BrightnessController::setBrightness(int brightness) {
    // Clamp brightness to valid range
    brightness = constrain(brightness, 0, 255);
    
    if (brightness != currentBrightness) {
        currentBrightness = brightness;
        
        // Apply to FastLED
        FastLED.setBrightness(brightness);
        
        // Update device state
        extern DeviceState deviceState;
        deviceState.brightness = brightness;
        
        // Trigger BLE callback to save preferences
        // BLEManager* ble = BLEManager::getInstance();
        // if (ble) {
        //     ble->triggerOnSettingChanged();
        // }
        
        
        // Notify external systems
        if (onBrightnessChanged) {
            onBrightnessChanged(brightness);
        }
        
        LOG_DEBUGF_COMPONENT("Brightness", "Set to: %d", brightness);
    }
}

void BrightnessController::startPulse(int target, unsigned long duration) {
    targetBrightness = constrain(target, 0, 255);
    pulseDuration = duration;
    pulseStartTime = millis();
    isPulsing = true;
    isFadeMode = false;
    
    LOG_DEBUGF_COMPONENT("Brightness", "Starting pulse to %d over %dms", targetBrightness, duration);
}

void BrightnessController::startFade(int target, unsigned long duration) {
    targetBrightness = constrain(target, 0, 255);
    pulseDuration = duration;
    pulseStartTime = millis();
    isPulsing = true;
    isFadeMode = true;
    
    LOG_DEBUGF_COMPONENT("Brightness", "Starting fade to %d over %dms", targetBrightness, duration);
}

void BrightnessController::stopPulse() {
    isPulsing = false;
    LOG_DEBUG_COMPONENT("Brightness", "Pulse/fade stopped");
}

void BrightnessController::update() {
    if (!isPulsing) {
        return;
    }
    
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - pulseStartTime;
    
    if (elapsed >= pulseDuration) {
        // Transition complete
        if (isFadeMode) {
            // For fade, stay at target brightness
            setBrightness(targetBrightness);
            LOG_DEBUGF_COMPONENT("Brightness", "Fade complete - now at %d", targetBrightness);
        } else {
            // For pulse, return to previous brightness (stored in targetBrightness)
            // We need to track the original brightness for pulses
            LOG_DEBUG_COMPONENT("Brightness", "Pulse complete");
        }
        
        isPulsing = false;
        
        // Notify completion
        if (onPulseComplete) {
            onPulseComplete();
        }
        return;
    }
    
    // Calculate current brightness using smooth interpolation
    float progress = (float)elapsed / (float)pulseDuration;
    
    float smoothProgress;
    if (isFadeMode) {
        // Linear interpolation for fade
        smoothProgress = progress;
    } else {
        // Smooth sine wave interpolation for natural pulsing effect
        smoothProgress = (sin(progress * 2 * PI - PI / 2) + 1) / 2;
    }
    
    // Calculate interpolated brightness
    int interpolatedBrightness = currentBrightness + (int)((targetBrightness - currentBrightness) * smoothProgress);
    setBrightness(interpolatedBrightness);
}

void BrightnessController::registerBLECharacteristic() {
    BLEManager* ble = BLEManager::getInstance();
    if (!ble) {
        LOG_WARN_COMPONENT("Brightness", "BLE not available");
        return;
    }
    
    BLECharacteristicRegistry* registry = ble->getRegistry();
    if (!registry) {
        LOG_WARN_COMPONENT("Brightness", "BLE registry not available");
        return;
    }
    
    LOG_DEBUG_COMPONENT("Brightness", "Registering BLE characteristic");
    registry->registerCharacteristic(brightnessCharacteristicInfo);
    LOG_DEBUG_COMPONENT("Brightness", "BLE characteristic registered successfully");
}

void BrightnessController::unregisterBLECharacteristic() {
    BLEManager* ble = BLEManager::getInstance();
    if (!ble) {
        return;
    }
    
    ble->getRegistry()->unregisterCharacteristic(brightnessCharacteristicInfo.characteristicUuid);
}

void BrightnessController::syncWithDeviceState(DeviceState& deviceState) {
    // Load brightness from device state without triggering BLE callback
    int brightness = constrain(deviceState.brightness, 0, 255);
    
    if (brightness != currentBrightness) {
        currentBrightness = brightness;
        
        // Apply to FastLED
        FastLED.setBrightness(brightness);
        
        // Update device state
        deviceState.brightness = brightness;
        
        // DON'T trigger BLE callback during sync (to avoid saving preferences during loading)
        // BLEManager* ble = BLEManager::getInstance();
        // if (ble) {
        //     ble->triggerOnSettingChanged();
        // }
        
        // Notify external systems
        if (onBrightnessChanged) {
            onBrightnessChanged(brightness);
        }
        
        LOG_DEBUGF_COMPONENT("Brightness", "Synced to: %d", brightness);
    }
}

void BrightnessController::updateDeviceState(DeviceState& deviceState) {
    // Update device state with current brightness
    deviceState.brightness = currentBrightness;
} 