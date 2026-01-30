#include "controllers/BrightnessController.h"
#include "hal/ble/BLEManager.h"
#include "freertos/LogManager.h"
#include "Utils.hpp"
#include <FastLED.h>

// Singleton instance
BrightnessController* BrightnessController::instance = nullptr;

BrightnessController::BrightnessController() 
    : currentBrightness(128), targetBrightness(128), isPulsing(false), 
      pulseStartTime(0), pulseDuration(0), isFadeMode(false), pulseStartBrightness(128),
      isPulseCycle(false), pulseCycleBase(128), pulseCyclePeak(255) {
    
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
        float mapped = getVaryingCurveMappedValue(rawVal / 255.0f, 1.f);
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
    if (instance == nullptr) {
        instance = new BrightnessController();
    }
}

void BrightnessController::destroy() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

BrightnessController* BrightnessController::getInstance() {
    return instance;
}

void BrightnessController::updateBrightness(int brightness) {
    // float mapped = getVaryingCurveMappedValue(brightness / 255.0f, 3.f);
    // int mappedVal = static_cast<int>(mapped * 255.0f + 0.5f);
    setBrightness(brightness);
}


void BrightnessController::setBrightness(int brightness) {
    // Clamp brightness to valid range
    brightness = constrain(brightness, 0, 255);
    
    // Apply curve mapping and set FastLED brightness
    // Always set it (even if value hasn't changed) to ensure FastLED gets updated during pulses
    float mapped = getVaryingCurveMappedValue(brightness / 255.0f, 1.f);
    int mappedVal = static_cast<int>(mapped * 255.0f + 0.5f);
    mappedVal = constrain(mappedVal, 0, 255);
    
    // Always update currentBrightness during pulses to ensure smooth interpolation
    // Only skip state updates if brightness hasn't changed (to avoid unnecessary callbacks)
    bool brightnessChanged = (brightness != currentBrightness);
    currentBrightness = brightness;
    
    // Always call FastLED.setBrightness() to ensure visual updates during pulses
    FastLED.setBrightness(mappedVal);
    
    // Only update device state and callbacks if brightness actually changed (not during pulse interpolation)
    if (brightnessChanged && !isPulsing) {
        // Update device state with RAW brightness (not mapped) so preferences stores raw value
        extern DeviceState deviceState;
        deviceState.brightness = brightness;  // Store raw value, not mapped
        
        // Notify callback
        if (onBrightnessChanged) {
            onBrightnessChanged(brightness);
        }
    }
}

void BrightnessController::startPulse(int target, unsigned long duration) {
    targetBrightness = constrain(target, 0, 255);
    pulseStartBrightness = currentBrightness;  // Save starting point for interpolation
    pulseDuration = duration;
    pulseStartTime = millis();
    isPulsing = true;
    isFadeMode = false;
}

void BrightnessController::startFade(int target, unsigned long duration) {
    targetBrightness = constrain(target, 0, 255);
    pulseStartBrightness = currentBrightness;  // Save starting point for interpolation
    pulseDuration = duration;
    pulseStartTime = millis();
    isPulsing = true;
    isFadeMode = true;
}

void BrightnessController::startPulseCycle(int baseBrightness, int peakBrightness, unsigned long duration) {
    // Stop any existing pulse - new pulse always overrides
    if (isPulsing) {
        LOG_DEBUGF_COMPONENT("BrightnessController", 
            "startPulseCycle: overriding existing pulse (isPulseCycle=%d, currentBrightness=%d)", 
            isPulseCycle, currentBrightness);
        stopPulse();
    }
    
    // Set up pulse cycle state - full cycle (base -> peak -> base) in one duration
    isPulseCycle = true;
    pulseCycleBase = baseBrightness;
    pulseCyclePeak = peakBrightness;
    pulseDuration = duration;  // Total duration for full cycle
    
    // Always start from base brightness for consistent behavior
    // Set base brightness immediately so we start from the correct value
    setBrightness(baseBrightness);
    
    // Start the cycle - update() will handle the full cycle with a single sine wave
    pulseStartTime = millis();
    pulseStartBrightness = baseBrightness;  // Start from base for consistent pulses
    targetBrightness = peakBrightness;  // Peak value (we'll animate base -> peak -> base)
    isPulsing = true;
    isFadeMode = false;  // Use sine wave
    
    LOG_DEBUGF_COMPONENT("BrightnessController", "startPulseCycle: base=%d, peak=%d, duration=%lu, currentBrightness=%d", 
                        baseBrightness, peakBrightness, duration, currentBrightness);
}

void BrightnessController::stopPulse() {
    if (isPulsing) {
        LOG_DEBUGF_COMPONENT("BrightnessController", "stopPulse: stopping pulse (isPulseCycle=%d, currentBrightness=%d)", 
                            isPulseCycle, currentBrightness);
    }
    isPulsing = false;
    isPulseCycle = false;
    onPulseComplete = nullptr;  // Clear callback to prevent stale callbacks from firing
}


void BrightnessController::update() {
    if (!isPulsing) {
        return;
    }
    
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - pulseStartTime;
    
    if (elapsed >= pulseDuration) {
        // Transition complete
        if (isPulseCycle) {
            // Pulse cycle complete (base -> peak -> base), return to base
            LOG_DEBUGF_COMPONENT("BrightnessController", 
                "PulseCycle complete: setting brightness to base=%d (current=%d)", 
                pulseCycleBase, currentBrightness);
            setBrightness(pulseCycleBase);
            isPulsing = false;
            isPulseCycle = false;
            
            // Notify completion
            if (onPulseComplete) {
                onPulseComplete();
            }
            return;
        } else {
            // Regular pulse or fade
            if (isFadeMode) {
                // For fade, stay at target brightness
                setBrightness(targetBrightness);
            }
            
            isPulsing = false;
            
            // Notify completion
            if (onPulseComplete) {
                onPulseComplete();
            }
            return;
        }
    }
    
    // Calculate current brightness using smooth interpolation
    float progress = (float)elapsed / (float)pulseDuration;
    
    float smoothProgress;
    if (isFadeMode) {
        // Linear interpolation for fade
        smoothProgress = progress;
    } else if (isPulseCycle) {
        // For pulse cycle: use sin(progress * PI) to go from 0 to 1 and back to 0
        // sin(0) = 0, sin(PI/2) = 1, sin(PI) = 0
        smoothProgress = sin(progress * PI);
    } else {
        // Regular pulse: smooth sine wave interpolation (0 to 1)
        smoothProgress = (sin(progress * 2 * PI - PI / 2) + 1) / 2;
    }
    
    // Calculate interpolated brightness
    int interpolatedBrightness;
    if (isPulseCycle) {
        // Pulse cycle: base -> peak -> base using sin curve
        // smoothProgress = sin(progress * PI) goes from 0 to 1 to 0
        // We want: start at base, peak at peakBrightness, end at baseBrightness
        int range = pulseCyclePeak - pulseCycleBase;
        interpolatedBrightness = pulseCycleBase + (int)(range * smoothProgress);
        
        // Log every 50ms to avoid spam
        static unsigned long lastLogTime = 0;
        if (currentTime - lastLogTime > 50) {
            LOG_DEBUGF_COMPONENT("BrightnessController", 
                "PulseCycle: elapsed=%lu, progress=%.3f, sin=%.3f, brightness=%d (base=%d, peak=%d, range=%d)",
                elapsed, progress, smoothProgress, interpolatedBrightness, pulseCycleBase, pulseCyclePeak, range);
            lastLogTime = currentTime;
        }
    } else {
        // Regular pulse/fade: from start to target
        interpolatedBrightness = pulseStartBrightness + (int)((targetBrightness - pulseStartBrightness) * smoothProgress);
    }
    
    setBrightness(interpolatedBrightness);
}

void BrightnessController::registerBLECharacteristic() {
    BLEManager* ble = BLEManager::getInstance();
    if (!ble) {
        return;
    }
    
    BLECharacteristicRegistry* registry = ble->getRegistry();
    if (!registry) {
        return;
    }
    
    registry->registerCharacteristic(brightnessCharacteristicInfo);
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
    // deviceState.brightness should be the raw value (since we store raw in preferences)
    LOG_DEBUGF_COMPONENT("BrightnessController", "syncWithDeviceState: deviceState.brightness=%d", deviceState.brightness);
    int brightness = constrain(deviceState.brightness, 0, 255);
    
    // Always apply curve mapping and set FastLED brightness (same as setBrightness does)
    // This ensures brightness is set correctly on boot
    float mapped = getVaryingCurveMappedValue(brightness / 255.0f, 1.f);
    int mappedVal = static_cast<int>(mapped * 255.0f + 0.5f);
    mappedVal = constrain(mappedVal, 0, 255);
    FastLED.setBrightness(mappedVal);
    
    // Update current brightness (raw value, before mapping)
    currentBrightness = brightness;
    
    // Keep device state as raw value (for consistency with setBrightness)
    deviceState.brightness = brightness;
    
    // DON'T trigger callbacks during sync (to avoid saving preferences during loading)
}

void BrightnessController::updateDeviceState(DeviceState& deviceState) {
    // Update device state with current brightness
    deviceState.brightness = currentBrightness;
} 