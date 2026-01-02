#pragma once

#include <ArduinoJson.h>
#include <memory>

/**
 * ICommandHandler - Interface for command processing
 * 
 * This interface allows WebSocketServer and other components to route
 * commands to different handlers (LEDManager, DeviceController, etc.)
 * without tight coupling.
 */
class ICommandHandler {
public:
    virtual ~ICommandHandler() = default;
    
    /**
     * Handle a JSON command
     * @param command The JSON command object to process
     * @return true if command was handled successfully, false otherwise
     */
    virtual bool handleCommand(const JsonObject& command) = 0;
    
    /**
     * Check if this handler supports queued command processing
     * @return true if handleQueuedCommand() should be used instead of handleCommand()
     */
    virtual bool supportsQueuing() const {
        return false;  // Default: no queuing support
    }
    
    /**
     * Handle a command with queuing support (optional)
     * 
     * This method is called when supportsQueuing() returns true.
     * The handler can queue the command for later processing in its main loop.
     * 
     * @param doc Shared pointer to the JSON document (can be queued safely)
     * @return true if command was queued successfully, false otherwise
     */
    virtual bool handleQueuedCommand(std::shared_ptr<DynamicJsonDocument> doc) {
        // Default implementation: extract JsonObject and call handleCommand
        if (doc) {
            return handleCommand(doc->as<JsonObject>());
        }
        return false;
    }
    
    /**
     * Get current brightness value (0-255)
     * Used for status reporting
     * @return Current brightness, or -1 if not applicable
     */
    virtual int getBrightness() const {
        return -1;  // Default: not applicable
    }
    
    /**
     * Get handler status string for logging/debugging
     * @return Status string describing handler state
     */
    virtual String getStatus() const {
        return "unknown";
    }
};
