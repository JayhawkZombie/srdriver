#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ACS712.h>  // RobTillaart's ACS712 library

// ACS712 variant types (for constructor convenience)
enum ACS712Variant {
    ACS712_5A,   // 185 mV/A at 5V
    ACS712_20A,  // 100 mV/A at 5V
    ACS712_30A   // 66 mV/A at 5V
};

// Configuration constants
namespace ACS712Config {
    constexpr uint16_t ESP32_ADC_RESOLUTION = 4095;
    constexpr float DEFAULT_LOWPASS_ALPHA = 0.1f;
    
    // Sensitivities for convenience
    constexpr float SENSITIVITY_5A = 185.0f;   // mV/A
    constexpr float SENSITIVITY_20A = 100.0f;  // mV/A
    constexpr float SENSITIVITY_30A = 66.0f;   // mV/A
}

/**
 * Wrapper class for ACS712 current sensor using RobTillaart's library
 * Adds: SD card persistence, filtering, polarity correction, error handling
 */
class ACS712CurrentSensor {
private:
    // Library backend
    ACS712* _acs712;
    
    // Our additions
    float _lowPassAlpha;
    float _lastReading;
    bool _filterInitialized;
    String _calibrationFilePath;
    bool _polarity_correction;  // Flip sign if wired backwards
    
    // Helper methods
    float _getSensitivityForVariant(ACS712Variant variant);

public:
    // Constructor - simplified to essential parameters
    ACS712CurrentSensor(uint8_t analogPin, 
                       ACS712Variant variant = ACS712_30A,
                       float supplyVoltage = 5.0f,
                       float adcReference = 3.3f);
    
    ~ACS712CurrentSensor();
    
    // Initialization
    void begin();
    
    // Reading methods with our enhancements
    float readCurrentDC_mA();                    // Direct library call with polarity fix
    float readCurrentDCFiltered_mA();           // Adds low-pass filtering
    
    // Configuration
    void setLowPassFilter(float alpha);
    void setPolarityCorrection(bool flipSign);   // Handle backwards wiring
    
    // SD card calibration persistence (our addition)
    bool loadCalibrationFromSD(const String& filepath = "/config/current_calibration.json");
    bool saveCalibrationToSD(const String& filepath = "/config/current_calibration.json");
    bool forceRecalibration();  // Clear saved calibration and re-calibrate
    
    // Library passthrough methods (for direct access)
    uint16_t autoCalibrate() { return _acs712 ? _acs712->autoMidPoint() : 0; }
    void setMidPoint(uint16_t midPoint) { if (_acs712) _acs712->setMidPoint(midPoint); }
    uint16_t getMidPoint() { return _acs712 ? _acs712->getMidPoint() : 0; }
    
    // Diagnostics
    void printDiagnostics();
    bool isReady() const { return _acs712 != nullptr; }
};


