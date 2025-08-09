#include "ACS712VoltageSensor.h"
#include <FS.h>
#include <SD.h>
#include <math.h>

// Constructor
ACS712VoltageSensor::ACS712VoltageSensor(uint8_t analogPin, 
                                       float supplyVoltage,
                                       float adcReference,
                                       float dividerRatio)
    : _analogPin(analogPin)
    , _supplyVoltage(supplyVoltage)
    , _adcReferenceVoltage(adcReference)
    , _adcMaxValue(VoltageConfig::ESP32_ADC_RESOLUTION)
    , _voltageDividerRatio(dividerRatio)
    , _zeroVoltageOffset(0.0)
    , _isCalibrated(false)
    , _averagingSamples(VoltageConfig::DEFAULT_AVERAGING_SAMPLES)
    , _lowPassAlpha(VoltageConfig::DEFAULT_LOWPASS_ALPHA)
    , _lastReading(0.0)
    , _filterInitialized(false)
    , _calibrationFilePath("/config/sensor_calibration.json")
    , _hasValidCalibration(false)
{
    _updateMaxVoltage();
}

void ACS712VoltageSensor::begin() {
    pinMode(_analogPin, INPUT);
    
    // Try to load calibration from SD card, but don't fail if SD unavailable
    bool sdCalibrationLoaded = false;
    try {
        sdCalibrationLoaded = loadCalibrationFromSD();
    } catch (...) {
        Serial.println("SD card not available for voltage sensor calibration, using defaults");
    }
    
    // If no calibration loaded, assume zero offset
    if (!_isCalibrated || !sdCalibrationLoaded) {
        _zeroVoltageOffset = 0.0;
        _isCalibrated = true;  // Voltage sensors don't necessarily need calibration
        Serial.printf("Using theoretical calibration for voltage sensor: %.3fV offset\n", _zeroVoltageOffset);
    }
}

// Configuration methods
void ACS712VoltageSensor::setSupplyVoltage(float voltage) {
    _supplyVoltage = voltage;
    _updateMaxVoltage();
}

void ACS712VoltageSensor::setADCReference(float voltage) {
    _adcReferenceVoltage = voltage;
    _updateMaxVoltage();
}

void ACS712VoltageSensor::setVoltageDividerRatio(float ratio) {
    _voltageDividerRatio = ratio;
    _updateMaxVoltage();
}

void ACS712VoltageSensor::setAveragingSamples(uint16_t samples) {
    _averagingSamples = samples;
}

void ACS712VoltageSensor::setLowPassFilter(float alpha) {
    _lowPassAlpha = alpha;
    _filterInitialized = false;  // Reset filter
}

// Private helper methods
void ACS712VoltageSensor::_updateMaxVoltage() {
    // Maximum measurable voltage is ADC reference * voltage divider ratio
    _maxMeasurableVoltage = _adcReferenceVoltage * _voltageDividerRatio;
}

uint16_t ACS712VoltageSensor::_readADCWithAveraging(uint16_t samples) {
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
void ACS712VoltageSensor::calibrateZeroPoint(uint16_t samples, bool saveToSD) {
    Serial.println("Calibrating ACS712 Voltage Sensor zero point...");
    Serial.println("Ensure ZERO voltage is applied to the sensor!");
    delay(3000);
    
    uint16_t rawADC = _readADCWithAveraging(samples);
    float rawVoltage = (rawADC * _adcReferenceVoltage) / _adcMaxValue;
    _zeroVoltageOffset = rawVoltage;
    _isCalibrated = true;
    _hasValidCalibration = true;
    
    Serial.printf("Calibration complete. Zero offset: %.4fV (ADC: %d)\n", 
                  _zeroVoltageOffset, rawADC);
    
    if (saveToSD) {
        saveCalibrationToSD();
    }
}

void ACS712VoltageSensor::setZeroOffset(float offset) {
    _zeroVoltageOffset = offset;
    _isCalibrated = true;
    _hasValidCalibration = true;
}

float ACS712VoltageSensor::getZeroOffset() const {
    return _zeroVoltageOffset;
}

bool ACS712VoltageSensor::isCalibrated() const {
    return _isCalibrated;
}

// SD card methods
bool ACS712VoltageSensor::loadCalibrationFromSD(const String& filepath) {
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
    
    // Look for voltage sensor data
    String sensorKey = String("voltage_sensor_") + String(_analogPin);
    if (doc["calibration"]["sensors"][sensorKey]) {
        JsonObject sensor = doc["calibration"]["sensors"][sensorKey];
        
        _zeroVoltageOffset = sensor["zero_offset"];
        _voltageDividerRatio = sensor["divider_ratio"];
        _supplyVoltage = sensor["supply_voltage"];
        _adcReferenceVoltage = sensor["adc_reference_voltage"] | _adcReferenceVoltage;
        
        _updateMaxVoltage();
        _isCalibrated = true;
        _hasValidCalibration = true;
        
        Serial.printf("Loaded calibration for pin %d: Offset=%.4fV, Ratio=%.2f:1\n",
                      _analogPin, _zeroVoltageOffset, _voltageDividerRatio);
        return true;
    }
    
    return false;
}

bool ACS712VoltageSensor::saveCalibrationToSD(const String& filepath) {
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
    
    String sensorKey = String("voltage_sensor_") + String(_analogPin);
    JsonObject sensor = doc["calibration"]["sensors"][sensorKey];
    
    sensor["type"] = "voltage_divider";
    sensor["analog_pin"] = _analogPin;
    sensor["supply_voltage"] = _supplyVoltage;
    sensor["adc_reference_voltage"] = _adcReferenceVoltage;
    sensor["divider_ratio"] = _voltageDividerRatio;
    sensor["zero_offset"] = _zeroVoltageOffset;
    sensor["max_measurable_voltage"] = _maxMeasurableVoltage;
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

void ACS712VoltageSensor::setCalibrationFilePath(const String& filepath) {
    _calibrationFilePath = filepath;
}

// Reading methods
float ACS712VoltageSensor::readVoltageDC_V() {
    uint16_t rawADC = analogRead(_analogPin);
    return adcToVoltage(rawADC);
}

float ACS712VoltageSensor::readVoltageDCFiltered_V() {
    uint16_t rawADC = _readADCWithAveraging(_averagingSamples);
    float voltage = adcToVoltage(rawADC);
    
    // Apply low-pass filter
    if (!_filterInitialized) {
        _lastReading = voltage;
        _filterInitialized = true;
    } else {
        _lastReading = _lowPassAlpha * voltage + (1.0 - _lowPassAlpha) * _lastReading;
    }
    
    return _lastReading;
}

float ACS712VoltageSensor::readVoltageRMS_V(uint16_t samples) {
    float sumOfSquares = 0.0;
    
    for (uint16_t i = 0; i < samples; i++) {
        uint16_t rawADC = analogRead(_analogPin);
        float voltage = adcToVoltage(rawADC);
        sumOfSquares += voltage * voltage;
        
        // Small delay for AC sampling
        delayMicroseconds(100);
    }
    
    return sqrt(sumOfSquares / samples);
}

// Raw reading methods
float ACS712VoltageSensor::readVoltageRaw() {
    uint16_t rawADC = analogRead(_analogPin);
    return (rawADC * _adcReferenceVoltage) / _adcMaxValue;
}

uint16_t ACS712VoltageSensor::readADCRaw() {
    return analogRead(_analogPin);
}

float ACS712VoltageSensor::adcToVoltage(uint16_t adcValue) {
    // Convert ADC value to actual voltage at the sensor input
    float sensorVoltage = (adcValue * _adcReferenceVoltage) / _adcMaxValue;
    
    // Correct for zero offset
    sensorVoltage -= _zeroVoltageOffset;
    
    // Scale by voltage divider ratio to get actual measured voltage
    float actualVoltage = sensorVoltage * _voltageDividerRatio;
    
    // Ensure we don't return negative voltages (unless it's a true negative reading)
    if (actualVoltage < 0.0 && _zeroVoltageOffset > 0.001) {
        actualVoltage = 0.0;
    }
    
    return actualVoltage;
}

// Error checking and diagnostics
bool ACS712VoltageSensor::hasValidCalibration() const {
    return _hasValidCalibration;
}

bool ACS712VoltageSensor::isReadingInRange(float voltage_V) const {
    return (voltage_V >= 0.0 && voltage_V <= _maxMeasurableVoltage);
}

void ACS712VoltageSensor::printRawDiagnostics() {
    uint16_t raw = analogRead(_analogPin);
    float rawVoltage = (raw * _adcReferenceVoltage) / _adcMaxValue;
    float actualVoltage = adcToVoltage(raw);
    
    Serial.printf("=== ACS712 Voltage Sensor Diagnostics (Pin %d) ===\n", _analogPin);
    Serial.printf("Raw ADC: %d (max: %d)\n", raw, _adcMaxValue);
    Serial.printf("Raw voltage: %.4fV\n", rawVoltage);
    Serial.printf("Zero offset: %.4fV\n", _zeroVoltageOffset);
    Serial.printf("Corrected voltage: %.4fV\n", rawVoltage - _zeroVoltageOffset);
    Serial.printf("Actual voltage: %.2fV (ratio: %.2f:1)\n", actualVoltage, _voltageDividerRatio);
    Serial.printf("Max measurable: %.1fV\n", _maxMeasurableVoltage);
    Serial.println("===============================================");
}

void ACS712VoltageSensor::printCalibrationInfo() {
    Serial.printf("=== ACS712 Voltage Sensor Calibration ===\n");
    Serial.printf("Pin: %d\n", _analogPin);
    Serial.printf("Supply voltage: %.2fV\n", _supplyVoltage);
    Serial.printf("ADC reference: %.2fV\n", _adcReferenceVoltage);
    Serial.printf("Voltage divider ratio: %.2f:1\n", _voltageDividerRatio);
    Serial.printf("Max measurable voltage: %.1fV\n", _maxMeasurableVoltage);
    Serial.printf("Zero offset: %.4fV\n", _zeroVoltageOffset);
    Serial.printf("Calibrated: %s\n", _isCalibrated ? "YES" : "NO");
    Serial.printf("Valid calibration: %s\n", _hasValidCalibration ? "YES" : "NO");
    Serial.println("========================================");
}

void ACS712VoltageSensor::printConfiguration() {
    Serial.printf("=== ACS712 Voltage Sensor Configuration ===\n");
    Serial.printf("Analog pin: %d\n", _analogPin);
    Serial.printf("Supply voltage: %.2fV\n", _supplyVoltage);
    Serial.printf("ADC reference: %.2fV\n", _adcReferenceVoltage);
    Serial.printf("ADC resolution: %d bits (%d max)\n", 12, _adcMaxValue);
    Serial.printf("Voltage divider ratio: %.2f:1\n", _voltageDividerRatio);
    Serial.printf("Max measurable voltage: %.1fV\n", _maxMeasurableVoltage);
    Serial.printf("Resolution per step: %.3fmV\n", (_adcReferenceVoltage / _adcMaxValue) * 1000);
    Serial.printf("Averaging samples: %d\n", _averagingSamples);
    Serial.printf("Low-pass alpha: %.3f\n", _lowPassAlpha);
    Serial.printf("Calibration file: %s\n", _calibrationFilePath.c_str());
    Serial.println("==========================================");
}

