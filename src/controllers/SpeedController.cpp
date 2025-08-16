#include "controllers/SpeedController.h"
#include "hal/ble/BLEManager.h"
#include <FastLED.h>

// Singleton instance
SpeedController* SpeedController::instance = nullptr;

SpeedController::SpeedController() 
    : currentSpeed(1.0f), targetSpeed(1.0f), isTransitioning(false), 
      transitionStartTime(0), transitionDuration(0) {
    
    // Initialize BLE characteristic info
    speedCharacteristicInfo.characteristicUuid = "a5fb3bc5-9633-4b85-8a42-7756f11ef7ac";
    speedCharacteristicInfo.descriptorUuid = "2901";
    speedCharacteristicInfo.formatDescriptorUuid = "2904";
    speedCharacteristicInfo.name = "Speed Control";
    speedCharacteristicInfo.description = "Controls LED pattern speed (0.0-20.0)";
    speedCharacteristicInfo.isWritable = true;
    speedCharacteristicInfo.isReadable = true;
    speedCharacteristicInfo.isNotifiable = true;
    speedCharacteristicInfo.maxValueLength = 8; // "20.000" + null terminator
    
    // Format data for 2904 descriptor
    speedCharacteristicInfo.formatData = {
        0x1A,      // UTF-8 String
        0,         // No exponent
        0x0000,    // No unit
        0x01,      // Bluetooth SIG namespace
        0x0000     // No description
    };
    
    // Set up BLE callbacks
    speedCharacteristicInfo.onWrite = [this](const unsigned char* value, size_t length) {
        // Handle speed write from BLE
        char buf[16];
        size_t len = std::min(sizeof(buf) - 1, length);
        memcpy(buf, value, len);
        buf[len] = '\0';
        String s(buf);
        float rawSpeed = s.toFloat();
        
        Serial.print("[Speed] Raw speed value: ");
        Serial.println(rawSpeed);
        
        // Scale from 0-255 range to 0-20 range (same as old handler)
        float scaledSpeed = rawSpeed / 255.0f * 20.0f;
        
        Serial.print("[Speed] Scaled speed: ");
        Serial.println(scaledSpeed);
        
        // Clamp to valid range (0.0 to 20.0)
        float clampedSpeed = constrain(scaledSpeed, 0.0f, 20.0f);
        
        Serial.print("[Speed] Clamped speed: ");
        Serial.println(clampedSpeed);
        
        // Update speed
        setSpeed(clampedSpeed);
        
        // Update the characteristic with the clamped value
        if (speedCharacteristicInfo.characteristic) {
            speedCharacteristicInfo.characteristic->writeValue(String(clampedSpeed, 3).c_str());
        }
    };
    
    speedCharacteristicInfo.onRead = [this]() -> String {
        return String(currentSpeed, 3);
    };
}

void SpeedController::initialize() {
    Serial.println("[Speed] initialize() called");
    if (instance == nullptr) {
        Serial.println("[Speed] Creating new instance...");
        instance = new SpeedController();
        Serial.println("[Speed] Controller initialized");
    } else {
        Serial.println("[Speed] Instance already exists");
    }
}

void SpeedController::destroy() {
    Serial.println("[Speed] destroy() called");
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
        Serial.println("[Speed] Controller destroyed");
    }
}

SpeedController* SpeedController::getInstance() {
    return instance;
}

void SpeedController::setSpeed(float speed) {
    // Clamp speed to valid range
    speed = constrain(speed, 0.0f, 20.0f);
    
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

void SpeedController::setSpeedWithTransition(float targetSpeed, unsigned long duration) {
    targetSpeed = constrain(targetSpeed, 0.0f, 20.0f);
    targetSpeed = targetSpeed;
    transitionDuration = duration;
    transitionStartTime = millis();
    isTransitioning = true;
    
    Serial.print("[Speed] Starting transition to ");
    Serial.print(targetSpeed);
    Serial.print(" over ");
    Serial.print(duration);
    Serial.println("ms");
}

void SpeedController::stopTransition() {
    isTransitioning = false;
    Serial.println("[Speed] Transition stopped");
}

void SpeedController::update() {
    if (!isTransitioning) {
        return;
    }
    
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - transitionStartTime;
    
    if (elapsed >= transitionDuration) {
        // Transition complete
        setSpeed(targetSpeed);
        isTransitioning = false;
        Serial.print("[Speed] Transition complete - now at ");
        Serial.println(targetSpeed);
        return;
    }
    
    // Calculate current speed using linear interpolation
    float progress = (float)elapsed / (float)transitionDuration;
    float interpolatedSpeed = currentSpeed + (targetSpeed - currentSpeed) * progress;
    updateSpeed(interpolatedSpeed);
}

void SpeedController::updateSpeed(float newSpeed) {
    if (newSpeed != currentSpeed) {
        currentSpeed = newSpeed;
        
        // Update global speedMultiplier for backward compatibility
        extern float speedMultiplier;
        speedMultiplier = currentSpeed;
        
        // Update device state
        extern DeviceState deviceState;
        deviceState.speedMultiplier = currentSpeed;
    }
}

void SpeedController::registerBLECharacteristic() {
    BLEManager* ble = BLEManager::getInstance();
    if (!ble) {
        Serial.println("[Speed] BLE not available");
        return;
    }
    
    BLECharacteristicRegistry* registry = ble->getRegistry();
    if (!registry) {
        Serial.println("[Speed] BLE registry not available");
        return;
    }
    
    Serial.println("[Speed] Registering BLE characteristic");
    registry->registerCharacteristic(speedCharacteristicInfo);
    Serial.println("[Speed] BLE characteristic registered successfully");
}

void SpeedController::unregisterBLECharacteristic() {
    BLEManager* ble = BLEManager::getInstance();
    if (!ble) {
        return;
    }
    
    ble->getRegistry()->unregisterCharacteristic(speedCharacteristicInfo.characteristicUuid);
}

void SpeedController::syncWithDeviceState(DeviceState& deviceState) {
    // Load speed from device state
    setSpeed(deviceState.speedMultiplier);
}

void SpeedController::updateDeviceState(DeviceState& deviceState) {
    // Update device state with current speed
    deviceState.speedMultiplier = currentSpeed;
} 