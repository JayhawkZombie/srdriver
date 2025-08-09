#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// Voltage sensor configuration constants
namespace VoltageConfig {
    constexpr uint16_t ESP32_ADC_RESOLUTION = 4095;
    constexpr float ESP32_DEFAULT_ADC_RANGE = 3.3;  // With ADC_11db attenuation
    constexpr uint16_t DEFAULT_AVERAGING_SAMPLES = 100;
    constexpr float DEFAULT_LOWPASS_ALPHA = 0.1;
    
    // Common voltage divider ratios
    constexpr float RATIO_1_TO_1 = 1.0;      // Direct connection (up to 3.3V)
    constexpr float RATIO_5_TO_1 = 5.0;      // Measure up to ~16.5V
    constexpr float RATIO_6_TO_1 = 6.06;     // Common for 20V range (based on 0.00489V resolution)
    constexpr float RATIO_10_TO_1 = 10.0;    // Measure up to ~33V
}

class ACS712VoltageSensor {
private:
    // Hardware configuration
    uint8_t _analogPin;
    float _supplyVoltage;        // Voltage supplied to sensor circuit (3.3V or 5V)
    float _adcReferenceVoltage;  // ESP32 ADC reference voltage
    uint16_t _adcMaxValue;       // ESP32 12-bit = 4095
    
    // Sensor characteristics
    float _voltageDividerRatio;  // e.g., 6:1 for measuring up to ~20V
    float _maxMeasurableVoltage; // Maximum voltage the sensor can measure
    float _zeroVoltageOffset;    // Calibrated offset voltage
    bool _isCalibrated;
    
    // Filtering/averaging parameters
    uint16_t _averagingSamples;
    float _lowPassAlpha;         // For simple low-pass filter
    float _lastReading;          // For low-pass filter
    bool _filterInitialized;
    
    // SD card calibration
    String _calibrationFilePath;
    bool _hasValidCalibration;
    
    // Internal helper methods
    void _updateMaxVoltage();
    uint16_t _readADCWithAveraging(uint16_t samples);

public:
    // Constructor with ESP32 defaults
    ACS712VoltageSensor(uint8_t analogPin, 
                       float supplyVoltage = 3.3,       // Default to 3.3V
                       float adcReference = 3.3,        // ESP32 default
                       float dividerRatio = 6.06);      // Based on typical 20V sensor
    
    // Initialization
    void begin();
    
    // Configuration methods
    void setSupplyVoltage(float voltage);
    void setADCReference(float voltage);
    void setVoltageDividerRatio(float ratio);
    void setAveragingSamples(uint16_t samples);
    void setLowPassFilter(float alpha);
    
    // Calibration methods
    void calibrateZeroPoint(uint16_t samples = 500, bool saveToSD = true);
    void setZeroOffset(float offset);
    float getZeroOffset() const;
    bool isCalibrated() const;
    
    // SD card calibration
    bool loadCalibrationFromSD(const String& filepath = "/config/sensor_calibration.json");
    bool saveCalibrationToSD(const String& filepath = "/config/sensor_calibration.json");
    void setCalibrationFilePath(const String& filepath);
    
    // Reading methods (returning V)
    float readVoltageDC_V();
    float readVoltageDCFiltered_V();
    float readVoltageRMS_V(uint16_t samples = 1000);
    
    // Raw reading methods
    float readVoltageRaw();      // Raw sensor output voltage (before divider correction)
    uint16_t readADCRaw();       // Raw ADC value
    float adcToVoltage(uint16_t adcValue);  // Convert ADC to actual measured voltage
    
    // Error checking and diagnostics
    bool hasValidCalibration() const;
    bool isReadingInRange(float voltage_V) const;
    void printRawDiagnostics();
    void printCalibrationInfo();
    void printConfiguration();
    
    // Getters for configuration
    float getSupplyVoltage() const { return _supplyVoltage; }
    float getADCReference() const { return _adcReferenceVoltage; }
    float getVoltageDividerRatio() const { return _voltageDividerRatio; }
    float getMaxMeasurableVoltage() const { return _maxMeasurableVoltage; }
    uint8_t getAnalogPin() const { return _analogPin; }
};

