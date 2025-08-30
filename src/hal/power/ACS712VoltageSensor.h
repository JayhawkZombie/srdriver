#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// Configuration constants
namespace VoltageConfig {
    constexpr uint16_t ESP32_ADC_RESOLUTION = 4095;
    constexpr float DEFAULT_LOWPASS_ALPHA = 0.1f;
    constexpr float PROVEN_VOLTAGE_DIVIDER_RATIO = 5.27f;  // Our proven working ratio
}

/**
 * Simplified voltage sensor class for voltage divider-based voltage sensing
 * Adds: SD card persistence, filtering, proven voltage scaling
 */
class ACS712VoltageSensor {
private:
    // Hardware configuration
    uint8_t _analogPin;
    float _adcReferenceVoltage;
    uint16_t _adcMaxValue;
    float _voltageDividerRatio;
    
    // Our additions
    float _lowPassAlpha;
    float _lastReading;
    bool _filterInitialized;
    String _calibrationFilePath;

public:
    // Constructor - simplified to essential parameters
    ACS712VoltageSensor(uint8_t analogPin, 
                       float adcReference = 3.3f,
                       float dividerRatio = VoltageConfig::PROVEN_VOLTAGE_DIVIDER_RATIO);
    
    // Initialization
    void begin();
    
    // Reading methods with our enhancements
    float readVoltageDC_V();                    // Direct voltage reading
    float readVoltageDCFiltered_V();           // Adds low-pass filtering
    
    // Configuration
    void setLowPassFilter(float alpha);
    void setVoltageDividerRatio(float ratio);   // Allow ratio adjustment
    
    // SD card calibration persistence (our addition)
    bool loadCalibrationFromSD(const String& filepath = "/config/voltage_calibration.json");
    bool saveCalibrationToSD(const String& filepath = "/config/voltage_calibration.json");
    
    // Diagnostics
    void printDiagnostics();
    bool isReady() const { return true; }  // Voltage sensor is always ready (no library dependency)
    
    // Raw access for debugging
    uint16_t readADCRaw() { return analogRead(_analogPin); }
    float readVoltageRaw() { return (analogRead(_analogPin) / (float)_adcMaxValue) * _adcReferenceVoltage; }
};

