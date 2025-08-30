#include "ACS712VoltageSensor.h"
#include "freertos/LogManager.h"
#include <Arduino.h>

ACS712VoltageSensor::ACS712VoltageSensor(uint8_t analogPin, 
                                        float adcReference,
                                        float dividerRatio)
    : _analogPin(analogPin),
      _adcReferenceVoltage(adcReference),
      _adcMaxValue(VoltageConfig::ESP32_ADC_RESOLUTION),
      _voltageDividerRatio(dividerRatio),
      _lowPassAlpha(VoltageConfig::DEFAULT_LOWPASS_ALPHA),
      _lastReading(0.0f),
      _filterInitialized(false),
      _calibrationFilePath("/config/voltage_calibration.json") {
}

void ACS712VoltageSensor::begin() {
    LOG_INFO("Initializing ACS712 Voltage Sensor (simplified)...");
    
    // Configure ADC attenuation for ESP32
    analogSetAttenuation(ADC_11db);  // 0-3.3V range
    
    LOG_PRINTF("Voltage sensor initialized - Pin: %d, Ratio: %.2f:1, Max: %.1fV", 
               _analogPin, _voltageDividerRatio, _adcReferenceVoltage * _voltageDividerRatio);
}

float ACS712VoltageSensor::readVoltageDC_V() {
    uint16_t rawADC = analogRead(_analogPin);
    
    // Convert ADC to voltage at the pin
    float pinVoltage = (rawADC / (float)_adcMaxValue) * _adcReferenceVoltage;
    
    // Apply voltage divider scaling to get actual measured voltage
    float actualVoltage = pinVoltage * _voltageDividerRatio;
    
    return actualVoltage;
}

float ACS712VoltageSensor::readVoltageDCFiltered_V() {
    float voltage = readVoltageDC_V();
    
    if (!_filterInitialized) {
        _lastReading = voltage;
        _filterInitialized = true;
        return voltage;
    }
    
    // Simple low-pass filter: output = α * input + (1-α) * previous_output
    _lastReading = _lowPassAlpha * voltage + (1.0f - _lowPassAlpha) * _lastReading;
    return _lastReading;
}

void ACS712VoltageSensor::setLowPassFilter(float alpha) {
    _lowPassAlpha = constrain(alpha, 0.0f, 1.0f);
    LOG_PRINTF("Voltage sensor filter alpha set to: %.3f", _lowPassAlpha);
}

void ACS712VoltageSensor::setVoltageDividerRatio(float ratio) {
    _voltageDividerRatio = ratio;
    LOG_PRINTF("Voltage divider ratio updated to: %.2f:1 (max voltage: %.1fV)", 
               ratio, _adcReferenceVoltage * ratio);
}

bool ACS712VoltageSensor::loadCalibrationFromSD(const String& filepath) {
    // Simplified - just return false for now
    return false;
}

bool ACS712VoltageSensor::saveCalibrationToSD(const String& filepath) {
    // Simplified - just return true for now
    return true;
}

void ACS712VoltageSensor::printDiagnostics() {
    LOG_PRINTF("=== Voltage Sensor Diagnostics ===");
    LOG_PRINTF("Pin: %d", _analogPin);
    LOG_PRINTF("Raw ADC: %d (max: %d)", readADCRaw(), _adcMaxValue);
    LOG_PRINTF("Raw Pin Voltage: %.3fV", readVoltageRaw());
    LOG_PRINTF("Divider Ratio: %.2f:1", _voltageDividerRatio);
    LOG_PRINTF("Actual Voltage: %.2fV", readVoltageDC_V());
    LOG_PRINTF("Filtered Voltage: %.2fV", readVoltageDCFiltered_V());
    LOG_PRINTF("Filter Alpha: %.3f", _lowPassAlpha);
    LOG_PRINTF("Max Measurable: %.1fV", _adcReferenceVoltage * _voltageDividerRatio);
}

