#pragma once

#include "ICommandHandler.h"
#include "freertos/LogManager.h"

/**
 * NullCommandHandler - Mock command handler for platforms without LED support
 * 
 * This handler accepts all commands but does nothing with them.
 * Useful for:
 * - CrowPanel (no LEDs)
 * - Testing WebSocket server without LED dependencies
 * - Development/debugging
 * 
 * Usage example:
 *   NullCommandHandler* nullHandler = new NullCommandHandler();
 *   wifiManager->setCommandHandler(nullHandler);
 *   wifiManager->startWebSocketServer();
 */
class NullCommandHandler : public ICommandHandler {
public:
    NullCommandHandler() {
        LOG_INFO_COMPONENT("NullCommandHandler", "NullCommandHandler created - commands will be accepted but not processed");
    }
    
    virtual ~NullCommandHandler() = default;
    
    /**
     * Accept commands but do nothing with them
     */
    bool handleCommand(const JsonObject& command) override {
        String type = "";
        if (command.containsKey("type")) {
            type = command["type"].as<String>();
        } else if (command.containsKey("t")) {
            type = command["t"].as<String>();
        }
        
        LOG_DEBUGF_COMPONENT("NullCommandHandler", "Received command (ignored): %s", type.c_str());
        return true;  // Return true to indicate "handled" (even though we did nothing)
    }
    
    /**
     * No queuing support needed
     */
    bool supportsQueuing() const override {
        return false;
    }
    
    /**
     * Brightness not applicable for null handler
     */
    int getBrightness() const override {
        return -1;  // Not applicable
    }
    
    /**
     * Return status indicating this is a null handler
     */
    String getStatus() const override {
        return "null_handler";
    }
};

