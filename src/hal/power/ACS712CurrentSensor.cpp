#include "ACS712CurrentSensor.h"
#include <FS.h>
#include <SD.h>
#include <math.h>

// Constructor
ACS712CurrentSensor::ACS712CurrentSensor(uint8_t analogPin, 
                                       float supplyVoltage,
                                       float adcReference,
                                       ACS712Variant variant)
    : _analogPin(analogPin)
    , _supplyVoltage(supplyVoltage)
    , _adcReferenceVoltage(adcReference)
    , _adcMaxValue(ACS712Config::ESP32_ADC_RESOLUTION)
    , _variant(variant)
    , _zeroCurrentVoltage(0.0)
    , _isCalibrated(false)
    , _averagingSamples(ACS712Config::DEFAULT_AVERAGING_SAMPLES)
    , _lowPassAlpha(ACS712Config::DEFAULT_LOWPASS_ALPHA)
    , _lastReading(0.0)
    , _filterInitialized(false)
    , _calibrationFilePath("/config/sensor_calibration.json")
    , _hasValidCalibration(false)
{
    // Set base sensitivity based on variant
    switch (_variant) {
        case ACS712_5A:
            _sensitivityBase = ACS712Config::SENSITIVITY_5A_5V;
            break;
        case ACS712_20A:
            _sensitivityBase = ACS712Config::SENSITIVITY_20A_5V;
            break;
        case ACS712_30A:
            _sensitivityBase = ACS712Config::SENSITIVITY_30A_5V;
            break;
        default:
            _sensitivityBase = ACS712Config::SENSITIVITY_30A_5V;
    }
    
    _updateSensitivity();
}

void ACS712CurrentSensor::begin() {
    pinMode(_analogPin, INPUT);
    
    // Try to load calibration from SD card, but don't fail if SD unavailable
    bool sdCalibrationLoaded = false;
    try {
        sdCalibrationLoaded = loadCalibrationFromSD();
    } catch (...) {
        Serial.println("SD card not available for current sensor calibration, using defaults");
    }
    
    // If no calibration loaded, calculate theoretical zero point
    if (!_isCalibrated || !sdCalibrationLoaded) {
        _zeroCurrentVoltage = _calculateZeroPoint();
        _isCalibrated = true;  // Mark as calibrated with theoretical values
        Serial.printf("Using theoretical calibration for current sensor: %.3fV zero point\n", _zeroCurrentVoltage);
    }
}

// Configuration methods
void ACS712CurrentSensor::setSupplyVoltage(float voltage) {
    _supplyVoltage = voltage;
    _updateSensitivity();
    if (!_isCalibrated) {
        _zeroCurrentVoltage = _calculateZeroPoint();
    }
}

void ACS712CurrentSensor::setADCReference(float voltage) {
    _adcReferenceVoltage = voltage;
}

void ACS712CurrentSensor::setAveragingSamples(uint16_t samples) {
    _averagingSamples = samples;
}

void ACS712CurrentSensor::setLowPassFilter(float alpha) {
    _lowPassAlpha = alpha;
    _filterInitialized = false;  // Reset filter
}

void ACS712CurrentSensor::setSensitivity(float mvPerAmp) {
    _actualSensitivity = mvPerAmp;
}

// Private helper methods
void ACS712CurrentSensor::_updateSensitivity() {
    // Sensitivity is ratiometric to supply voltage
    _actualSensitivity = _sensitivityBase * (_supplyVoltage / 5.0);
}

float ACS712CurrentSensor::_calculateZeroPoint() const {
    // Theoretical zero point is VCC/2
    return _supplyVoltage / 2.0;
}

uint16_t ACS712CurrentSensor::_readADCWithAveraging(uint16_t samples) {
    uint32_t sum = 0;
    for (uint16_t i = 0; i < samples; i++) {
        sum += analogRead(_analogPin);
        if (samples > 10) {
            delay(1);  // Small delay for stability with many samples
        }
    }
    return sum / samples;
}

// Calibration methods
void ACS712CurrentSensor::calibrateZeroPoint(uint16_t samples, bool saveToSD) {
    Serial.println("Calibrating ACS712 Current Sensor zero point...");
    Serial.println("Ensure NO current is flowing through the sensor!");
    delay(3000);
    
    uint16_t rawADC = _readADCWithAveraging(samples);
    _zeroCurrentVoltage = (rawADC * _adcReferenceVoltage) / _adcMaxValue;
    _isCalibrated = true;
    _hasValidCalibration = true;
    
    Serial.printf("Calibration complete. Zero point: %.4fV (ADC: %d)\n", 
                  _zeroCurrentVoltage, rawADC);
    
    if (saveToSD) {
        saveCalibrationToSD();
    }
}

void ACS712CurrentSensor::setZeroPoint(float voltage) {
    _zeroCurrentVoltage = voltage;
    _isCalibrated = true;
    _hasValidCalibration = true;
}

float ACS712CurrentSensor::getZeroPoint() const {
    return _zeroCurrentVoltage;
}

bool ACS712CurrentSensor::isCalibrated() const {
    return _isCalibrated;
}

// SD card methods
bool ACS712CurrentSensor::loadCalibrationFromSD(const String& filepath) {
    if (!SD.exists(filepath)) {
        Serial.println("No calibration file found on SD card");
        return false;
    }
    
    File file = SD.open(filepath, FILE_READ);
    if (!file) {
        Serial.println("Failed to open calibration file");
        return false;
    }
    
    String jsonString = file.readString();
    file.close();
    
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, jsonString);
    
    if (error) {
        Serial.printf("Failed to parse calibration JSON: %s\n", error.c_str());
        return false;
    }
    
    // Look for current sensor data
    String sensorKey = String("current_sensor_") + String(_analogPin);
    if (doc["calibration"]["sensors"][sensorKey]) {
        JsonObject sensor = doc["calibration"]["sensors"][sensorKey];
        
        _zeroCurrentVoltage = sensor["zero_point_voltage"];
        _actualSensitivity = sensor["sensitivity_mv_per_amp"];
        _supplyVoltage = sensor["supply_voltage"];
        _adcReferenceVoltage = sensor["adc_reference_voltage"] | _adcReferenceVoltage;
        
        _isCalibrated = true;
        _hasValidCalibration = true;
        
        Serial.printf("Loaded calibration for pin %d: Zero=%.4fV, Sensitivity=%.2fmV/A\n",
                      _analogPin, _zeroCurrentVoltage, _actualSensitivity);
        return true;
    }
    
    return false;
}

bool ACS712CurrentSensor::saveCalibrationToSD(const String& filepath) {
    // Load existing calibration file or create new one
    DynamicJsonDocument doc(2048);
    
    if (SD.exists(filepath)) {
        File file = SD.open(filepath, FILE_READ);
        if (file) {
            String jsonString = file.readString();
            file.close();
            deserializeJson(doc, jsonString);
        }
    }
    
    // Update or create calibration data
    doc["calibration"]["version"] = "1.0";
    doc["calibration"]["timestamp"] = millis();  // You might want to use real timestamp
    
    String sensorKey = String("current_sensor_") + String(_analogPin);
    JsonObject sensor = doc["calibration"]["sensors"][sensorKey];
    
    sensor["type"] = "ACS712_30A";  // You could make this dynamic based on _variant
    sensor["analog_pin"] = _analogPin;
    sensor["supply_voltage"] = _supplyVoltage;
    sensor["adc_reference_voltage"] = _adcReferenceVoltage;
    sensor["zero_point_voltage"] = _zeroCurrentVoltage;
    sensor["sensitivity_mv_per_amp"] = _actualSensitivity;
    sensor["calibration_samples"] = _averagingSamples;
    sensor["calibration_timestamp"] = millis();
    
    // Write to SD card
    File file = SD.open(filepath, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open calibration file for writing");
        return false;
    }
    
    serializeJsonPretty(doc, file);
    file.close();
    
    Serial.printf("Saved calibration to %s\n", filepath.c_str());
    return true;
}

void ACS712CurrentSensor::setCalibrationFilePath(const String& filepath) {
    _calibrationFilePath = filepath;
}

// Reading methods
float ACS712CurrentSensor::readCurrentDC_mA() {
    uint16_t rawADC = analogRead(_analogPin);
    float voltage = (rawADC * _adcReferenceVoltage) / _adcMaxValue;
    return voltageToCurrentDC_mA(voltage);
}

float ACS712CurrentSensor::readCurrentDCFiltered_mA() {
    uint16_t rawADC = _readADCWithAveraging(_averagingSamples);
    float voltage = (rawADC * _adcReferenceVoltage) / _adcMaxValue;
    float current = voltageToCurrentDC_mA(voltage);
    
    // Apply low-pass filter
    if (!_filterInitialized) {
        _lastReading = current;
        _filterInitialized = true;
    } else {
        _lastReading = _lowPassAlpha * current + (1.0 - _lowPassAlpha) * _lastReading;
    }
    
    return _lastReading;
}

float ACS712CurrentSensor::readCurrentRMS_mA(uint16_t samples) {
    float sumOfSquares = 0.0;
    
    for (uint16_t i = 0; i < samples; i++) {
        uint16_t rawADC = analogRead(_analogPin);
        float voltage = (rawADC * _adcReferenceVoltage) / _adcMaxValue;
        float current = voltageToCurrentDC_mA(voltage);
        sumOfSquares += current * current;
        
        // Small delay for AC sampling
        delayMicroseconds(100);
    }
    
    return sqrt(sumOfSquares / samples);
}

// Raw reading methods
float ACS712CurrentSensor::readVoltageRaw() {
    uint16_t rawADC = analogRead(_analogPin);
    return (rawADC * _adcReferenceVoltage) / _adcMaxValue;
}

uint16_t ACS712CurrentSensor::readADCRaw() {
    return analogRead(_analogPin);
}

float ACS712CurrentSensor::voltageToCurrentDC_mA(float voltage) {
    if (!_isCalibrated) {
        Serial.println("Warning: Sensor not calibrated!");
    }
    
    // Convert voltage difference to current in mA
    float voltageDiff = voltage - _zeroCurrentVoltage;
    float currentA = voltageDiff / (_actualSensitivity / 1000.0);  // Convert mV/A to V/A
    return currentA * 1000.0;  // Convert A to mA
}

// Error checking and diagnostics
bool ACS712CurrentSensor::hasValidCalibration() const {
    return _hasValidCalibration;
}

bool ACS712CurrentSensor::isReadingInRange(float current_mA) const {
    // Determine max current based on variant
    float maxCurrentA = 30.0;  // Default to 30A
    switch (_variant) {
        case ACS712_5A: maxCurrentA = 5.0; break;
        case ACS712_20A: maxCurrentA = 20.0; break;
        case ACS712_30A: maxCurrentA = 30.0; break;
    }
    
    float maxCurrentMA = maxCurrentA * 1000.0;
    return (current_mA >= -maxCurrentMA && current_mA <= maxCurrentMA);
}

void ACS712CurrentSensor::printRawDiagnostics() {
    uint16_t raw = analogRead(_analogPin);
    float voltage = (raw * _adcReferenceVoltage) / _adcMaxValue;
    float current = voltageToCurrentDC_mA(voltage);
    
    Serial.printf("=== ACS712 Current Sensor Diagnostics (Pin %d) ===\n", _analogPin);
    Serial.printf("Raw ADC: %d (max: %d)\n", raw, _adcMaxValue);
    Serial.printf("Voltage: %.4fV\n", voltage);
    Serial.printf("Current: %.1fmA\n", current);
    Serial.printf("Zero point: %.4fV\n", _zeroCurrentVoltage);
    Serial.printf("Voltage difference: %.4fV\n", voltage - _zeroCurrentVoltage);
    Serial.println("===============================================");
}

void ACS712CurrentSensor::printCalibrationInfo() {
    Serial.printf("=== ACS712 Current Sensor Calibration ===\n");
    Serial.printf("Pin: %d\n", _analogPin);
    Serial.printf("Variant: %s\n", 
                  _variant == ACS712_5A ? "5A" : 
                  _variant == ACS712_20A ? "20A" : "30A");
    Serial.printf("Supply voltage: %.2fV\n", _supplyVoltage);
    Serial.printf("ADC reference: %.2fV\n", _adcReferenceVoltage);
    Serial.printf("Base sensitivity: %.1fmV/A\n", _sensitivityBase);
    Serial.printf("Actual sensitivity: %.1fmV/A\n", _actualSensitivity);
    Serial.printf("Zero point: %.4fV\n", _zeroCurrentVoltage);
    Serial.printf("Calibrated: %s\n", _isCalibrated ? "YES" : "NO");
    Serial.printf("Valid calibration: %s\n", _hasValidCalibration ? "YES" : "NO");
    Serial.println("========================================");
}

void ACS712CurrentSensor::printConfiguration() {
    Serial.printf("=== ACS712 Current Sensor Configuration ===\n");
    Serial.printf("Analog pin: %d\n", _analogPin);
    Serial.printf("Supply voltage: %.2fV\n", _supplyVoltage);
    Serial.printf("ADC reference: %.2fV\n", _adcReferenceVoltage);
    Serial.printf("ADC resolution: %d bits (%d max)\n", 12, _adcMaxValue);
    Serial.printf("Averaging samples: %d\n", _averagingSamples);
    Serial.printf("Low-pass alpha: %.3f\n", _lowPassAlpha);
    Serial.printf("Calibration file: %s\n", _calibrationFilePath.c_str());
    Serial.println("==========================================");
}