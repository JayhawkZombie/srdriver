#pragma once

#include <ArduinoJson.h>
#include "freertos/LogManager.h"
#include <WebSocketsServer.h>
#include "ICommandHandler.h"

/**
 * SRWebSocketServer - Self-contained WebSocket server for command control
 * 
 * Features:
 * - Self-contained with update() method for easy integration
 * - Command routing to ICommandHandler (LEDManager, DeviceController, etc.)
 * - Status broadcasting to connected clients
 * - Client management and connection handling
 */
class SRWebSocketServer {
public:
    SRWebSocketServer(ICommandHandler* commandHandler, uint16_t port = 8080);
    ~SRWebSocketServer();
    
    // Lifecycle
    void start();
    void stop();
    void update(); // Main "tick" method - call this every loop
    
    // Status
    bool isRunning() const;
    uint8_t getConnectedClients() const;
    String getStatus() const;
    
    // Broadcasting
    void broadcastStatus();
    void broadcastMessage(const String& message);
    void sendToClient(uint8_t clientId, const String& message);

private:
    ICommandHandler* _commandHandler;
    WebSocketsServer* _wsServer;
    uint16_t _port;
    bool _isRunning;
    uint8_t _connectedClients;
    unsigned long _lastStatusUpdate;
    
    // WebSocket event handling
    void handleWebSocketEvent(uint8_t clientId, WStype_t type, uint8_t* payload, size_t length);
    void processMessage(uint8_t clientId, const String& message);
    void processLEDCommand(const JsonObject& command);
    void sendStatusUpdate(uint8_t clientId);
    
    // JSON command processing
    void handleEffectCommand(const JsonObject& command);
    void handleBrightnessCommand(const JsonObject& command);
    void handleStatusCommand(uint8_t clientId);
    
    // Status generation
    String generateStatusJSON() const;
};
