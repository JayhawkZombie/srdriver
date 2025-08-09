#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// ACS712 variant types
enum ACS712Variant {
    ACS712_5A,   // 185 mV/A at 5V
    ACS712_20A,  // 100 mV/A at 5V
    ACS712_30A   // 66 mV/A at 5V
};

// Configuration constants
namespace ACS712Config {
    constexpr uint16_t ESP32_ADC_RESOLUTION = 4095;
    constexpr float ESP32_DEFAULT_ADC_RANGE = 3.3;  // With ADC_11db attenuation
    constexpr uint16_t DEFAULT_AVERAGING_SAMPLES = 100;
    constexpr float DEFAULT_LOWPASS_ALPHA = 0.1;
    
    // Base sensitivities at 5V supply
    constexpr float SENSITIVITY_5A_5V = 185.0;   // mV/A
    constexpr float SENSITIVITY_20A_5V = 100.0;  // mV/A
    constexpr float SENSITIVITY_30A_5V = 66.0;   // mV/A
}

class ACS712CurrentSensor {
private:
    // Hardware configuration
    uint8_t _analogPin;
    float _supplyVoltage;        // Voltage supplied to the ACS712 (3.3V or 5V)
    float _adcReferenceVoltage;  // ESP32 ADC reference voltage
    uint16_t _adcMaxValue;       // ESP32 12-bit = 4095
    ACS712Variant _variant;
    
    // Sensor characteristics
    float _sensitivityBase;      // Base sensitivity at 5V (mV/A)
    float _actualSensitivity;    // Actual sensitivity at current supply voltage (mV/A)
    float _zeroCurrentVoltage;   // Calibrated zero point voltage
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
    void _updateSensitivity();
    float _calculateZeroPoint() const;
    uint16_t _readADCWithAveraging(uint16_t samples);

public:
    // Constructor with ESP32 defaults
    ACS712CurrentSensor(uint8_t analogPin, 
                       float supplyVoltage = 3.3,      // Default to 3.3V
                       float adcReference = 3.3,       // ESP32 default
                       ACS712Variant variant = ACS712_30A);
    
    // Initialization
    void begin();
    
    // Configuration methods
    void setSupplyVoltage(float voltage);
    void setADCReference(float voltage);
    void setAveragingSamples(uint16_t samples);
    void setLowPassFilter(float alpha);
    void setSensitivity(float mvPerAmp);
    
    // Calibration methods
    void calibrateZeroPoint(uint16_t samples = 500, bool saveToSD = true);
    void setZeroPoint(float voltage);
    float getZeroPoint() const;
    bool isCalibrated() const;
    
    // SD card calibration
    bool loadCalibrationFromSD(const String& filepath = "/config/sensor_calibration.json");
    bool saveCalibrationToSD(const String& filepath = "/config/sensor_calibration.json");
    void setCalibrationFilePath(const String& filepath);
    
    // Reading methods (returning mA)
    float readCurrentDC_mA();
    float readCurrentDCFiltered_mA();
    float readCurrentRMS_mA(uint16_t samples = 1000);
    
    // Raw reading methods
    float readVoltageRaw();      // Sensor's actual output voltage
    uint16_t readADCRaw();       // Raw ADC value
    float voltageToCurrentDC_mA(float voltage); // Convert voltage to current
    
    // Error checking and diagnostics
    bool hasValidCalibration() const;
    bool isReadingInRange(float current_mA) const;
    void printRawDiagnostics();
    void printCalibrationInfo();
    void printConfiguration();
    
    // Getters for configuration
    float getSupplyVoltage() const { return _supplyVoltage; }
    float getADCReference() const { return _adcReferenceVoltage; }
    float getActualSensitivity() const { return _actualSensitivity; }
    ACS712Variant getVariant() const { return _variant; }
    uint8_t getAnalogPin() const { return _analogPin; }
};


