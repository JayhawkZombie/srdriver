#pragma once

#include <Arduino.h>

// Forward declaration
class DisplayBuffer;

/**
 * DisplayQueue - Queue for managing display content across multiple regions
 * 
 * Manages both banner messages (yellow region) and main display content (blue region).
 * Banner messages have priority and always display over main content.
 * Main display requests are first-come-first-served.
 */
class DisplayQueue {
public:
    // Display system states
    enum class DisplayState {
        STARTUP,        // During setup, before DisplayTask is ready
        READY,          // DisplayTask is running and ready
        ERROR           // DisplayTask failed to start
    };
    
    // Task name constants - easy to add new ones
    static const String TASK_BLE;
    static const String TASK_SYSTEM;
    static const String TASK_SENSOR;
    static const String TASK_SD_CARD;
    static const String TASK_NETWORK;
    static const String TASK_STORAGE;
    
    // Singleton access
    static DisplayQueue& getInstance();
    
    // Banner message API (existing functionality)
    void requestBannerMessage(const String& taskName, const String& message);
    void clearBannerMessage(const String& taskName);
    
    // Safe banner API - checks if display system is ready
    bool safeRequestBannerMessage(const String& taskName, const String& message);
    bool safeClearBannerMessage(const String& taskName);
    
    // NEW: Main display area API
    void requestMainDisplay(DisplayBuffer& buffer, const String& taskName);
    void releaseMainDisplay(const String& taskName);
    
    // Safe main display API - checks if display system is ready
    bool safeRequestMainDisplay(DisplayBuffer& buffer, const String& taskName);
    bool safeRequestMainDisplay(const String& taskName);  // NEW: No buffer needed
    bool safeReleaseMainDisplay(const String& taskName);
    
    // Display system state management
    void setDisplayState(DisplayState state);
    DisplayState getDisplayState() const { return _displayState; }
    bool isDisplayTaskReady() const { return _displayState == DisplayState::READY; }
    
    // Query current state
    bool hasActiveMessage() const;
    bool hasActiveMainDisplay() const { return _hasActiveMainDisplay; }
    const String& getCurrentTaskName() const { return _currentTaskName; }
    const String& getCurrentMessage() const { return _currentMessage; }
    const String& getMainDisplayOwner() const { return _mainDisplayOwner; }
    String getFullBannerText() const;
    
    // NEW: Get current main display buffer
    const DisplayBuffer* getCurrentMainDisplay() const { return _currentMainDisplay; }
    
    // Timeout management (banner messages only)
    void setMessageTimeout(uint32_t timeoutMs);
    uint32_t getMessageTimeout() const { return _messageTimeout; }
    
    // Internal timeout checking (called by DisplayTask)
    void checkMessageTimeout();
    
private:
    DisplayQueue();
    ~DisplayQueue() = default;
    DisplayQueue(const DisplayQueue&) = delete;
    DisplayQueue& operator=(const DisplayQueue&) = delete;
    
    DisplayState _displayState;
    
    // Banner message state
    String _currentTaskName;
    String _currentMessage;
    uint32_t _messageStartTime;
    uint32_t _messageTimeout;
    bool _hasActiveMessage;
    
    // NEW: Main display state
    String _mainDisplayOwner;
    DisplayBuffer* _currentMainDisplay;
    bool _hasActiveMainDisplay;
}; 