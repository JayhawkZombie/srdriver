#pragma once

#include <Arduino.h>

// Microphone configuration
#define DEFAULT_MIC_PIN A0
#define DEFAULT_SAMPLE_RATE 8000
#define DEFAULT_SAMPLE_WINDOW 50
#define DEFAULT_ADC_RESOLUTION 12
#define DEFAULT_ADC_ATTENUATION ADC_11db
#define DEFAULT_AUDIO_THRESHOLD 5  // Minimum audio level to trigger detection

class MAX4466_MicrophoneComponent {
private:
    int micPin;
    int sampleRate;
    int sampleWindow;
    int adcResolution;
    adc_attenuation_t adcAttenuation;
    int audioThreshold;
    
    // Audio processing variables
    int micValue;
    int micMin;
    int micMax;
    int micAvg;
    int dcBias;
    int audioLevel;
    int peakLevel;
    
    // Timing variables
    unsigned long lastSampleTime;
    unsigned long lastPeakTime;
    unsigned long lastResetTime;
    const unsigned long SAMPLE_INTERVAL;  // Calculated from sample rate
    
    // Sample buffer for averaging
    int* sampleBuffer;
    int sampleIndex;
    long sampleSum;
    
    bool initialized;
    bool autoCalibrate;
    bool audioDetected;
    bool clippingDetected;

public:
    // Constructors
    MAX4466_MicrophoneComponent(int pin = DEFAULT_MIC_PIN)
        : micPin(pin),
          sampleRate(DEFAULT_SAMPLE_RATE),
          sampleWindow(DEFAULT_SAMPLE_WINDOW),
          adcResolution(DEFAULT_ADC_RESOLUTION),
          adcAttenuation(DEFAULT_ADC_ATTENUATION),
          audioThreshold(DEFAULT_AUDIO_THRESHOLD),
          micValue(0),
          micMin(4095),
          micMax(0),
          micAvg(0),
          dcBias(0),
          audioLevel(0),
          peakLevel(0),
          lastSampleTime(0),
          lastPeakTime(0),
          lastResetTime(0),
          SAMPLE_INTERVAL(1000000 / DEFAULT_SAMPLE_RATE),
          sampleBuffer(nullptr),
          sampleIndex(0),
          sampleSum(0),
          initialized(false),
          autoCalibrate(true),
          audioDetected(false),
          clippingDetected(false)
    {
    }
    
    MAX4466_MicrophoneComponent(int pin, int rate, int window)
        : micPin(pin),
          sampleRate(rate),
          sampleWindow(window),
          adcResolution(DEFAULT_ADC_RESOLUTION),
          adcAttenuation(DEFAULT_ADC_ATTENUATION),
          audioThreshold(DEFAULT_AUDIO_THRESHOLD),
          micValue(0),
          micMin(4095),
          micMax(0),
          micAvg(0),
          dcBias(0),
          audioLevel(0),
          peakLevel(0),
          lastSampleTime(0),
          lastPeakTime(0),
          lastResetTime(0),
          SAMPLE_INTERVAL(1000000 / rate),
          sampleBuffer(nullptr),
          sampleIndex(0),
          sampleSum(0),
          initialized(false),
          autoCalibrate(true),
          audioDetected(false),
          clippingDetected(false)
    {
    }
    
    // Destructor
    ~MAX4466_MicrophoneComponent() {
        if (sampleBuffer) {
            delete[] sampleBuffer;
        }
    }
    
    // Initialization
    bool begin() {
        return begin(micPin, sampleRate, sampleWindow);
    }
    
    bool begin(int pin, int rate, int window) {
        micPin = pin;
        sampleRate = rate;
        sampleWindow = window;
        
        // Configure ADC
        analogReadResolution(adcResolution);
        analogSetAttenuation(adcAttenuation);
        
        // Allocate sample buffer
        if (sampleBuffer) {
            delete[] sampleBuffer;
        }
        sampleBuffer = new int[sampleWindow];
        if (!sampleBuffer) {
            Serial.println("Failed to allocate sample buffer");
            return false;
        }
        
        // Initialize sample buffer
        for (int i = 0; i < sampleWindow; i++) {
            sampleBuffer[i] = 0;
        }
        
        // Calibrate DC bias
        if (autoCalibrate) {
            calibrateDCBias();
        }
        
        initialized = true;
        return true;
    }
    
    // Configuration
    void setSampleRate(int rate) {
        sampleRate = rate;
        if (initialized) {
            // Recalculate sample interval
            const_cast<unsigned long&>(SAMPLE_INTERVAL) = 1000000 / rate;
        }
    }
    
    void setSampleWindow(int window) {
        sampleWindow = window;
        // Note: Would need to reallocate buffer if already initialized
    }
    
    void setADCResolution(int resolution) {
        adcResolution = resolution;
        if (initialized) {
            analogReadResolution(resolution);
        }
    }
    
    void setADCAttenuation(adc_attenuation_t attenuation) {
        adcAttenuation = attenuation;
        if (initialized) {
            analogSetAttenuation(attenuation);
        }
    }
    
    void setAutoCalibrate(bool enable) {
        autoCalibrate = enable;
    }
    
    void setAudioThreshold(int threshold) {
        audioThreshold = threshold;
    }

    int getAudioThreshold() const {
        return audioThreshold;
    }

    // Audio reading functions
    void update() {
        if (!initialized) return;
        
        unsigned long currentTime = micros();
        
        // Sample at the specified rate
        if (currentTime - lastSampleTime >= SAMPLE_INTERVAL) {
            lastSampleTime = currentTime;
            
            // Read microphone value
            micValue = analogRead(micPin);
            
            // Update min/max values
            if (micValue < micMin) micMin = micValue;
            if (micValue > micMax) micMax = micValue;
            
            // Update sample buffer
            sampleSum -= sampleBuffer[sampleIndex];
            sampleBuffer[sampleIndex] = micValue;
            sampleSum += micValue;
            sampleIndex = (sampleIndex + 1) % sampleWindow;
            
            // Calculate average
            micAvg = sampleSum / sampleWindow;
            
            // Calculate audio level based on deviation from DC bias
            int peakDeviation = max(abs(micMax - dcBias), abs(micMin - dcBias));
            audioLevel = map(peakDeviation, 0, 2048, 0, 100);
            
            // Update peak level
            if (audioLevel > peakLevel) {
                peakLevel = audioLevel;
                lastPeakTime = currentTime;
            }
            
            // Decay peak level over time
            if (currentTime - lastPeakTime > 100000) { // 100ms decay
                peakLevel = max(0, peakLevel - 2);
            }
            
            // Check for audio detection
            audioDetected = (audioLevel > audioThreshold);
            
            // Check for clipping
            clippingDetected = (micMin <= 0 || micMax >= 4095);
            
            // Reset min/max every second
            if (currentTime - lastResetTime >= 1000000) {
                lastResetTime = currentTime;
                resetMinMax();
            }
        }
    }
    
    // Getter functions
    int getRawValue() const { return micValue; }
    int getAudioLevel() const { return audioLevel; }
    int getPeakLevel() const { return peakLevel; }
    int getDCBias() const { return dcBias; }
    int getMinValue() const { return micMin; }
    int getMaxValue() const { return micMax; }
    int getAverageValue() const { return micAvg; }
    
    // Calibration functions
    void calibrateDCBias(int samples = 100) {
        Serial.println("Calibrating DC bias...");
        long biasSum = 0;
        for (int i = 0; i < samples; i++) {
            biasSum += analogRead(micPin);
            delay(1);
        }
        dcBias = biasSum / samples;
        Serial.print("DC Bias: ");
        Serial.print(dcBias);
        Serial.print(" (");
        Serial.print((dcBias * 3.3) / 4095, 2);
        Serial.println("V)");
    }
    
    void resetMinMax() {
        micMin = 4095;
        micMax = 0;
    }
    
    // Status functions
    bool isInitialized() const { return initialized; }
    bool isAudioDetected() const { return audioDetected; }
    bool isClipping() const { return clippingDetected; }
    
    // Utility functions
    float getVoltage() const {
        return (micValue * 3.3) / 4095;
    }
    
    int getVolumeDB() const {
        // Approximate dB calculation (not calibrated)
        if (audioLevel == 0) return -60;
        return map(audioLevel, 0, 100, -60, 0);
    }
    
    // Print current status
    void printStatus() const {
        Serial.print("Raw: ");
        Serial.print(micValue);
        Serial.print(", DC: ");
        Serial.print(dcBias);
        Serial.print(", Min: ");
        Serial.print(micMin);
        Serial.print(", Max: ");
        Serial.print(micMax);
        Serial.print(", Avg: ");
        Serial.print(micAvg);
        Serial.print(", Vol: ");
        Serial.print(audioLevel);
        Serial.println("%");
    }
};
