#pragma once

#include <Arduino.h>

/**
 * DisplayQueue - Simple queue for managing display banner messages
 * 
 * Allows tasks to request banner messages in the yellow region of the display.
 * Only one message can be active at a time - first come, first served.
 */
class DisplayQueue {
public:
    // Display system states
    enum class DisplayState {
        STARTUP,        // During setup, before display task is ready
        READY,          // Display task is running and ready
        ERROR           // Display task failed to start
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
    
    // Core API
    void requestBannerMessage(const String& taskName, const String& message);
    void clearBannerMessage(const String& taskName);
    
    // Safe API - checks if display system is ready
    bool safeRequestBannerMessage(const String& taskName, const String& message);
    bool safeClearBannerMessage(const String& taskName);
    
    // Display system state management
    void setDisplayState(DisplayState state);
    DisplayState getDisplayState() const { return _displayState; }
    bool isDisplayTaskReady() const { return _displayState == DisplayState::READY; }  // Generic - works for any display task
    
    // Query current state
    bool hasActiveMessage() const;
    const String& getCurrentTaskName() const { return _currentTaskName; }
    const String& getCurrentMessage() const { return _currentMessage; }
    String getFullBannerText() const;
    
    // Timeout management
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
    String _currentTaskName;
    String _currentMessage;
    uint32_t _messageStartTime;
    uint32_t _messageTimeout;
    bool _hasActiveMessage;
}; 