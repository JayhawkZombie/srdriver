#include "ACS712CurrentSensor.h"
#include "freertos/LogManager.h"
#include <Arduino.h>

ACS712CurrentSensor::ACS712CurrentSensor(uint8_t analogPin, 
                                        ACS712Variant variant,
                                        float supplyVoltage,
                                        float adcReference)
    : _acs712(nullptr),
      _lowPassAlpha(ACS712Config::DEFAULT_LOWPASS_ALPHA),
      _lastReading(0.0f),
      _filterInitialized(false),
      _calibrationFilePath("/config/current_calibration.json"),
      _polarity_correction(true) {  // Default to true since your sensor is wired backwards
    
    // Create the library object with appropriate sensitivity
    float sensitivity = _getSensitivityForVariant(variant);
    _acs712 = new ACS712(analogPin, supplyVoltage, ACS712Config::ESP32_ADC_RESOLUTION, sensitivity);
}

ACS712CurrentSensor::~ACS712CurrentSensor() {
    if (_acs712) {
        delete _acs712;
        _acs712 = nullptr;
    }
}

float ACS712CurrentSensor::_getSensitivityForVariant(ACS712Variant variant) {
    switch (variant) {
        case ACS712_5A:  return ACS712Config::SENSITIVITY_5A;
        case ACS712_20A: return ACS712Config::SENSITIVITY_20A;
        case ACS712_30A: return ACS712Config::SENSITIVITY_30A;
        default:         return ACS712Config::SENSITIVITY_30A;
    }
}

void ACS712CurrentSensor::begin() {
    LOG_INFO("Initializing ACS712 Current Sensor (simplified)...");
    
    if (!_acs712) {
        LOG_ERROR("ACS712 library object not created!");
        return;
    }
    
    // Configure ADC attenuation for ESP32
    analogSetAttenuation(ADC_11db);  // 0-3.3V range
    
    // Auto-calibrate using the library
    LOG_INFO("Auto-calibrating current sensor (ensure no current flowing)...");
    uint16_t midPoint = _acs712->autoMidPoint();
    LOG_PRINTF("Auto-calibration complete, midpoint: %d", midPoint);
    
    LOG_INFO("Current sensor initialized successfully");
}

float ACS712CurrentSensor::readCurrentDC_mA() {
    if (!_acs712) {
        LOG_WARN("Current sensor not initialized");
        return -999.0f;  // Error value
    }
    
    // Get reading from library
    float current = _acs712->mA_DC();
    
    // Apply polarity correction if needed
    if (_polarity_correction) {
        current = -current;
    }
    
    return current;
}

float ACS712CurrentSensor::readCurrentDCFiltered_mA() {
    float current = readCurrentDC_mA();
    
    if (current == -999.0f) {
        return current;  // Pass through error value
    }
    
    if (!_filterInitialized) {
        _lastReading = current;
        _filterInitialized = true;
        return current;
    }
    
    // Simple low-pass filter: output = α * input + (1-α) * previous_output
    _lastReading = _lowPassAlpha * current + (1.0f - _lowPassAlpha) * _lastReading;
    return _lastReading;
}

void ACS712CurrentSensor::setLowPassFilter(float alpha) {
    _lowPassAlpha = constrain(alpha, 0.0f, 1.0f);
    LOG_PRINTF("Current sensor filter alpha set to: %.3f", _lowPassAlpha);
}

void ACS712CurrentSensor::setPolarityCorrection(bool flipSign) {
    _polarity_correction = flipSign;
    LOG_PRINTF("Current sensor polarity correction: %s", flipSign ? "ENABLED" : "DISABLED");
}

bool ACS712CurrentSensor::loadCalibrationFromSD(const String& filepath) {
    // Simplified - just return false for now
    return false;
}

bool ACS712CurrentSensor::saveCalibrationToSD(const String& filepath) {
    // Simplified - just return true for now
    return true;
}

void ACS712CurrentSensor::printDiagnostics() {
    if (!_acs712) {
        LOG_ERROR("=== Current Sensor NOT INITIALIZED ===");
        return;
    }
    
    LOG_PRINTF("=== Current Sensor Diagnostics ===");
    LOG_PRINTF("Library Ready: YES");
    LOG_PRINTF("Midpoint: %d", _acs712->getMidPoint());
    LOG_PRINTF("Current Reading: %.1f mA", readCurrentDC_mA());
    LOG_PRINTF("Filtered Reading: %.1f mA", readCurrentDCFiltered_mA());
    LOG_PRINTF("Polarity Correction: %s", _polarity_correction ? "ENABLED" : "DISABLED");
    LOG_PRINTF("Filter Alpha: %.3f", _lowPassAlpha);
}