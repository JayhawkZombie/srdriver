#pragma once

#include "../InputDevice.h"
#include "MAX4466_MicrophoneComponent.h"

/**
 * Input device implementation for microphones
 * Handles audio input with level detection and clipping detection
 */
class MicrophoneInputDevice : public InputDevice {
private:
    MAX4466_MicrophoneComponent* mic;
    String name;
    int lastAudioLevel;
    int currentAudioLevel;
    bool lastAudioDetected;
    bool currentAudioDetected;
    bool lastClippingDetected;
    bool currentClippingDetected;
    bool changed;
    bool initialized;
    
public:
    /**
     * Constructor
     * @param deviceName Name of the device
     * @param micPin Analog pin number
     */
    MicrophoneInputDevice(const String& deviceName, int micPin) 
        : name(deviceName), changed(false), initialized(false) {
        mic = new MAX4466_MicrophoneComponent(micPin);
        initialized = mic->begin();
        if (initialized) {
            lastAudioLevel = mic->getAudioLevel();
            currentAudioLevel = lastAudioLevel;
            lastAudioDetected = mic->isAudioDetected();
            currentAudioDetected = lastAudioDetected;
            lastClippingDetected = mic->isClipping();
            currentClippingDetected = lastClippingDetected;
        }
    }
    
    /**
     * Constructor with custom parameters
     * @param deviceName Name of the device
     * @param micPin Analog pin number
     * @param sampleRate Sample rate in Hz
     * @param sampleWindow Sample window in milliseconds
     */
    MicrophoneInputDevice(const String& deviceName, int micPin, int sampleRate, int sampleWindow) 
        : name(deviceName), changed(false), initialized(false) {
        mic = new MAX4466_MicrophoneComponent(micPin, sampleRate, sampleWindow);
        initialized = mic->begin();
        if (initialized) {
            lastAudioLevel = mic->getAudioLevel();
            currentAudioLevel = lastAudioLevel;
            lastAudioDetected = mic->isAudioDetected();
            currentAudioDetected = lastAudioDetected;
            lastClippingDetected = mic->isClipping();
            currentClippingDetected = lastClippingDetected;
        }
    }
    
    /**
     * Destructor
     */
    ~MicrophoneInputDevice() {
        if (mic) {
            delete mic;
            mic = nullptr;
        }
    }
    
    /**
     * Poll the microphone for current state
     */
    void poll() override {
        if (!initialized || !mic) return;
        
        // Update microphone
        mic->update();
        
        // Store previous values
        lastAudioLevel = currentAudioLevel;
        lastAudioDetected = currentAudioDetected;
        lastClippingDetected = currentClippingDetected;
        
        // Get current values
        currentAudioLevel = mic->getAudioLevel();
        currentAudioDetected = mic->isAudioDetected();
        currentClippingDetected = mic->isClipping();
        
        // Check if anything changed
        changed = (currentAudioLevel != lastAudioLevel) || 
                  (currentAudioDetected != lastAudioDetected) ||
                  (currentClippingDetected != lastClippingDetected);
    }
    
    /**
     * Check if microphone state has changed
     * @return true if state changed, false otherwise
     */
    bool hasChanged() const override { 
        return changed; 
    }
    
    /**
     * Reset the changed flag
     */
    void resetChanged() override { 
        changed = false; 
    }
    
    /**
     * Get device type
     * @return "microphone"
     */
    String getDeviceType() const override { 
        return "microphone"; 
    }
    
    /**
     * Get device name
     * @return Device name
     */
    String getName() const override { 
        return name; 
    }
    
    /**
     * Get raw event data
     * @return Pointer to current audio level
     */
    void* getEventData() override { 
        return &currentAudioLevel; 
    }
    
    /**
     * Get raw value (audio level)
     * @return Raw audio level
     */
    int getRawValue() const override {
        return mic ? mic->getRawValue() : 0;
    }
    
    /**
     * Get mapped value (volume in dB)
     * @return Volume in decibels
     */
    int getMappedValue() const override {
        return mic ? mic->getVolumeDB() : 0;
    }
    
    /**
     * Get current audio level
     * @return Current audio level
     */
    int getAudioLevel() const {
        return currentAudioLevel;
    }
    
    /**
     * Get peak audio level
     * @return Peak audio level
     */
    int getPeakLevel() const {
        return mic ? mic->getPeakLevel() : 0;
    }
    
    /**
     * Check if audio is detected
     * @return true if audio detected, false otherwise
     */
    bool isAudioDetected() const {
        return currentAudioDetected;
    }
    
    /**
     * Check if audio is clipping
     * @return true if clipping detected, false otherwise
     */
    bool isClipping() const {
        return currentClippingDetected;
    }
    
    /**
     * Get DC bias
     * @return DC bias value
     */
    int getDCBias() const {
        return mic ? mic->getDCBias() : 0;
    }
    
    /**
     * Get voltage
     * @return Voltage in volts
     */
    float getVoltage() const {
        return mic ? mic->getVoltage() : 0.0f;
    }
    
    /**
     * Get volume in decibels
     * @return Volume in dB
     */
    int getVolumeDB() const {
        return mic ? mic->getVolumeDB() : 0;
    }
    
    /**
     * Set audio threshold
     * @param threshold Audio detection threshold
     */
    void setAudioThreshold(int threshold) {
        if (mic) {
            mic->setAudioThreshold(threshold);
        }
    }
    
    /**
     * Get audio threshold
     * @return Current audio threshold
     */
    int getAudioThreshold() const {
        return mic ? mic->getAudioThreshold() : 5;
    }
    
    /**
     * Calibrate DC bias
     * @param samples Number of samples for calibration
     */
    void calibrateDCBias(int samples = 100) {
        if (mic) {
            mic->calibrateDCBias(samples);
        }
    }
    
    /**
     * Reset min/max values
     */
    void resetMinMax() {
        if (mic) {
            mic->resetMinMax();
        }
    }
}; 