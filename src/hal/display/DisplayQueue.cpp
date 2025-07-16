#include "DisplayQueue.h"
#include "DisplayBuffer.h"
#include <freertos/LogManager.h>

// Define static constants
const String DisplayQueue::TASK_BLE = "BLE";
const String DisplayQueue::TASK_SYSTEM = "System";
const String DisplayQueue::TASK_SENSOR = "Sensor";
const String DisplayQueue::TASK_SD_CARD = "SD Card";
const String DisplayQueue::TASK_NETWORK = "Network";
const String DisplayQueue::TASK_STORAGE = "Storage";

DisplayQueue& DisplayQueue::getInstance() {
    static DisplayQueue instance;
    return instance;
}

DisplayQueue::DisplayQueue() 
    : _displayState(DisplayState::STARTUP)
    , _messageStartTime(0)
    , _messageTimeout(0)  // Default: no timeout
    , _hasActiveMessage(false)
    , _currentMainDisplay(nullptr)
    , _hasActiveMainDisplay(false) {
}

void DisplayQueue::setDisplayState(DisplayState state) {
    _displayState = state;
}

bool DisplayQueue::safeRequestBannerMessage(const String& taskName, const String& message) {
    if (_displayState != DisplayState::READY) {
        return false; // Display system not ready
    }
    
    requestBannerMessage(taskName, message);
    return true; // Request accepted
}

bool DisplayQueue::safeClearBannerMessage(const String& taskName) {
    if (_displayState != DisplayState::READY) {
        return false; // Display system not ready
    }
    
    clearBannerMessage(taskName);
    return true; // Clear request accepted
}

// NEW: Safe main display API
bool DisplayQueue::safeRequestMainDisplay(DisplayBuffer& buffer, const String& taskName) {
    if (_displayState != DisplayState::READY) {
        return false; // Display system not ready
    }
    
    requestMainDisplay(buffer, taskName);
    return true; // Request accepted
}

bool DisplayQueue::safeRequestMainDisplay(const String& taskName) {
    if (_displayState != DisplayState::READY) {
        return false; // Display system not ready
    }
    
    // In the new system, we don't need to pass a buffer
    // The FrameComposer will handle the rendering
    // Just mark that this task wants to show system stats
    LOG_DEBUGF("System stats display requested: %s", taskName.c_str());
    return true; // Request accepted
}

bool DisplayQueue::safeReleaseMainDisplay(const String& taskName) {
    if (_displayState != DisplayState::READY) {
        return false; // Display system not ready
    }
    
    releaseMainDisplay(taskName);
    return true; // Release request accepted
}

void DisplayQueue::requestBannerMessage(const String& taskName, const String& message) {
    // Only accept messages if DisplayTask is ready
    if (_displayState != DisplayState::READY) {
        return; // Ignore requests during startup
    }
    
    // Simple first-come-first-served approach
    // If no message is active, this task gets the banner
    if (!_hasActiveMessage) {
        _currentTaskName = taskName;
        _currentMessage = message;
        _messageStartTime = millis();
        _hasActiveMessage = true;
        LOG_DEBUGF("Banner message set: %s: '%s' (timeout: %d ms)", 
                  taskName.c_str(), message.c_str(), _messageTimeout);
    } else {
        LOG_DEBUGF("Banner message ignored: %s: '%s' (current: %s: '%s')", 
                  taskName.c_str(), message.c_str(), 
                  _currentTaskName.c_str(), _currentMessage.c_str());
    }
    // If a message is already active, this request is ignored
}

void DisplayQueue::clearBannerMessage(const String& taskName) {
    // Only the task that set the message can clear it
    if (_hasActiveMessage && _currentTaskName == taskName) {
        _hasActiveMessage = false;
        _currentTaskName = "";
        _currentMessage = "";
        LOG_DEBUGF("Banner message cleared: %s", taskName.c_str());
    }
}

// NEW: Main display area management
void DisplayQueue::requestMainDisplay(DisplayBuffer& buffer, const String& taskName) {
    // Only accept requests if DisplayTask is ready
    if (_displayState != DisplayState::READY) {
        return; // Ignore requests during startup
    }
    
    // Simple first-come-first-served approach for main display
    // If no main display is active, this task gets it
    if (!_hasActiveMainDisplay) {
        _mainDisplayOwner = taskName;
        _currentMainDisplay = &buffer;
        _hasActiveMainDisplay = true;
        LOG_DEBUGF("Main display requested: %s", taskName.c_str());
    } else {
        LOG_DEBUGF("Main display request ignored: %s (current owner: %s)", 
                  taskName.c_str(), _mainDisplayOwner.c_str());
    }
    // If main display is already active, this request is ignored
}

void DisplayQueue::releaseMainDisplay(const String& taskName) {
    // Only the task that owns the main display can release it
    if (_hasActiveMainDisplay && _mainDisplayOwner == taskName) {
        _hasActiveMainDisplay = false;
        _mainDisplayOwner = "";
        _currentMainDisplay = nullptr;
        LOG_DEBUGF("Main display released: %s", taskName.c_str());
    }
}

String DisplayQueue::getFullBannerText() const {
    if (!_hasActiveMessage) {
        return "SRDriver";
    }
    
    // Format: "TaskName: Message"
    String fullText = _currentTaskName + ": " + _currentMessage;
    
    // Truncate if too long for display (SSD1306 is 128 pixels wide, ~16 chars at size 1)
    const int maxLength = 16;
    if (fullText.length() > maxLength) {
        fullText = fullText.substring(0, maxLength - 3) + "...";
    }
    
    return fullText;
}

bool DisplayQueue::hasActiveMessage() const { 
    // Add debug logging to see what's happening
    static uint32_t lastDebugTime = 0;
    uint32_t now = millis();
    if (now - lastDebugTime > 5000) {  // Log every 5 seconds
        LOG_DEBUGF("hasActiveMessage check: _hasActiveMessage=%s, _messageTimeout=%d", 
                  _hasActiveMessage ? "true" : "false", _messageTimeout);
        if (_hasActiveMessage) {
            LOG_DEBUGF("Active message: '%s: %s' (started %d ms ago)", 
                      _currentTaskName.c_str(), _currentMessage.c_str(), 
                      now - _messageStartTime);
        }
        lastDebugTime = now;
    }
    
    return _hasActiveMessage; 
}

void DisplayQueue::checkMessageTimeout() {
    if (_hasActiveMessage && _messageTimeout > 0) {
        uint32_t now = millis();
        uint32_t elapsed = now - _messageStartTime;
        static uint32_t lastTimeoutDebug = 0;
        if (now - lastTimeoutDebug > 1000) {
            LOG_PRINTF("Timeout check: elapsed=%d ms, timeout=%d ms, remaining=%d ms", 
                     elapsed, _messageTimeout, _messageTimeout - elapsed);
            lastTimeoutDebug = now;
        }
        if (elapsed > _messageTimeout) {
            LOG_PRINTF("Auto-clearing expired banner message from %s: '%s' (timeout: %d ms)", 
                     _currentTaskName.c_str(), _currentMessage.c_str(), _messageTimeout);
            _hasActiveMessage = false;
            _currentTaskName = "";
            _currentMessage = "";
        }
    } else if (_hasActiveMessage && _messageTimeout == 0) {
        static uint32_t lastNoTimeoutDebug = 0;
        uint32_t now = millis();
        if (now - lastNoTimeoutDebug > 5000) {
            LOG_WARNF("Active message has no timeout set: %s: '%s'", 
                     _currentTaskName.c_str(), _currentMessage.c_str());
            lastNoTimeoutDebug = now;
        }
    }
}

void DisplayQueue::setMessageTimeout(uint32_t timeoutMs) {
    _messageTimeout = timeoutMs;
    LOG_PRINTF("Message timeout set to %d ms", timeoutMs);
} 