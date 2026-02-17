#include "WebSocketServer.h"
#include "lights/LEDManager.h"
#include "freertos/LogManager.h"
#include "controllers/BrightnessController.h"
#include <WebSocketsServer.h>
#include "../PatternManager.h"
#include "DeviceState.h"
#include "DeviceInfo.h"

SRWebSocketServer::SRWebSocketServer(ICommandHandler* commandHandler, uint16_t port) 
    : _commandHandler(commandHandler), _port(port), _isRunning(false), _lastStatusUpdate(0) {
    _wsServer = nullptr;
}

SRWebSocketServer::~SRWebSocketServer() {
    stop();
}

void SRWebSocketServer::start() {
    LOG_DEBUG_COMPONENT("WebSocketServer", "SRWebSocketServer::start() called");
    
    if (_isRunning) {
        LOG_WARN_COMPONENT("WebSocketServer", "WebSocket server already running");
        return;
    }
    
    LOG_DEBUGF_COMPONENT("WebSocketServer", "SRWebSocketServer: Creating WebSocketsServer on port %d", _port);
    try {
        _wsServer = new WebSocketsServer(_port);
        LOG_DEBUG_COMPONENT("WebSocketServer", "SRWebSocketServer: WebSocketsServer instance created");
        
        LOG_DEBUG_COMPONENT("WebSocketServer", "SRWebSocketServer: Setting up event handler...");
        _wsServer->onEvent([this](uint8_t clientId, WStype_t type, uint8_t* payload, size_t length) {
            this->handleWebSocketEvent(clientId, type, payload, length);
        });
        LOG_DEBUG_COMPONENT("WebSocketServer", "SRWebSocketServer: Event handler set");
        
        LOG_DEBUG_COMPONENT("WebSocketServer", "SRWebSocketServer: Starting server...");
        _wsServer->begin();
        LOG_DEBUG_COMPONENT("WebSocketServer", "SRWebSocketServer: Server started");
        
        // CRITICAL: Enable heartbeat to detect dead connections quickly
        // This prevents the 20-minute TCP timeout delay when clients are unplugged
        // Ping every 30 seconds, timeout after 10 seconds, disconnect after 3 failed pongs
        _wsServer->enableHeartbeat(30000, 10000, 3);
        LOG_DEBUG_COMPONENT("WebSocketServer", "SRWebSocketServer: Heartbeat enabled (30s ping, 10s timeout, 3 failures)");
        
        _isRunning = true;
        _lastStatusUpdate = 0;
        
        LOG_INFOF_COMPONENT("WebSocketServer", "SRWebSocketServer: WebSocket server started on port %d with heartbeat", _port);
    } catch (const std::exception& e) {
        LOG_ERRORF_COMPONENT("WebSocketServer", "SRWebSocketServer: Exception in start(): %s", e.what());
        if (_wsServer) {
            delete _wsServer;
            _wsServer = nullptr;
        }
        throw;
    } catch (...) {
        LOG_ERROR_COMPONENT("WebSocketServer", "SRWebSocketServer: Unknown exception in start()");
        if (_wsServer) {
            delete _wsServer;
            _wsServer = nullptr;
        }
        throw;
    }
}

void SRWebSocketServer::stop() {
    if (!_isRunning || !_wsServer) return;
    
    _wsServer->close();
    delete _wsServer;
    _wsServer = nullptr;
    _isRunning = false;
    
    LOG_INFO_COMPONENT("WebSocketServer", "WebSocket server stopped");
}

void SRWebSocketServer::update() {
    if (!_isRunning || !_wsServer) {
        return;
    }
    
    // Let WebSocketsServer handle its internal processing
    // This may trigger handleWebSocketEvent() callbacks, including DISCONNECTED events
    try {
        _wsServer->loop();
    } catch (...) {
        LOG_ERROR_COMPONENT("WebSocketServer", "Exception in _wsServer->loop()");
        return;
    }
    
    // Handle periodic status updates (every 5 seconds)
    unsigned long now = millis();
    if (now - _lastStatusUpdate > 5000) {
        // Re-check everything after loop() - state may have changed
        if (_wsServer && _isRunning) {
            // Query library for actual client count - no manual counter
            uint8_t clientCount = getConnectedClients();
            if (clientCount > 0) {
                LOG_DEBUGF_COMPONENT("WebSocketServer", "WebSocket update: broadcasting status to %d clients", clientCount);
                broadcastStatus();
            }
            _lastStatusUpdate = now;
        }
    }
}

bool SRWebSocketServer::isRunning() const {
    return _isRunning;
}

uint8_t SRWebSocketServer::getConnectedClients() const {
    // CRITICAL: Always query library directly - no manual tracking!
    if (!_isRunning || !_wsServer) {
        return 0;
    }
    try {
        return _wsServer->connectedClients();
    } catch (...) {
        LOG_ERROR_COMPONENT("WebSocketServer", "Exception querying connected clients");
        return 0;
    }
}

String SRWebSocketServer::getStatus() const {
    if (!_isRunning) return "stopped";
    return String("running on port ") + String(_port) + " with " + String(getConnectedClients()) + " clients";
}

void SRWebSocketServer::broadcastStatus() {
    if (!_isRunning) {
        return;
    }
    
    // Query library for actual count - no manual counter
    uint8_t clientCount = getConnectedClients();
    if (clientCount == 0) {
        return;  // No clients, nothing to broadcast
    }
    
    if (clientCount > 10) {
        LOG_ERRORF_COMPONENT("WebSocketServer", "Unexpected client count: %d", clientCount);
        return;  // Safety check
    }
    
    String statusJSON = generateStatusJSON();
    broadcastMessage(statusJSON);
}

void SRWebSocketServer::broadcastMessage(const String& message) {
    if (!_isRunning || !_wsServer) {
        return;
    }
    
    // Query library for actual count before broadcasting
    uint8_t clientCount = getConnectedClients();
    if (clientCount == 0 || clientCount > 10) {
        if (clientCount > 10) {
            LOG_ERRORF_COMPONENT("WebSocketServer", "Invalid client count in broadcast: %d", clientCount);
        }
        return;
    }
    
    if (message.length() == 0) {
        LOG_WARN_COMPONENT("WebSocketServer", "Attempted to broadcast empty message");
        return;
    }
    
    // Double-check _wsServer is still valid
    if (!_wsServer) {
        LOG_WARN_COMPONENT("WebSocketServer", "WebSocket server pointer became null before broadcast");
        return;
    }
    
    try {
        // CRITICAL: Check return value - broadcastTXT returns false on failure
        bool success = _wsServer->broadcastTXT(message.c_str(), message.length());
        if (success) {
            LOG_DEBUGF_COMPONENT("WebSocketServer", "Broadcasted message to %d clients", clientCount);
        } else {
            LOG_WARNF_COMPONENT("WebSocketServer", "Failed to broadcast message to %d clients - connections may be dead", clientCount);
            // Library will handle cleanup of dead connections via heartbeat
        }
    } catch (...) {
        LOG_ERROR_COMPONENT("WebSocketServer", "Exception in broadcastTXT");
    }
}

// CRITICAL: Helper to safely check if we can send to a client
bool SRWebSocketServer::canSendToClient(uint8_t clientId) const {
    if (!_isRunning || !_wsServer) {
        return false;
    }
    
    try {
        // Verify client is actually connected before attempting to send
        return _wsServer->clientIsConnected(clientId);
    } catch (...) {
        LOG_ERRORF_COMPONENT("WebSocketServer", "Exception checking client %d connection", clientId);
        return false;
    }
}

void SRWebSocketServer::sendToClient(uint8_t clientId, const String& message) {
    // CRITICAL: Always verify client is connected before sending!
    if (!canSendToClient(clientId)) {
        LOG_WARNF_COMPONENT("WebSocketServer", "Cannot send to client %d - not connected or server not running", clientId);
        return;
    }
    
    if (message.length() == 0) {
        LOG_WARN_COMPONENT("WebSocketServer", "Attempted to send empty message to client");
        return;
    }
    
    // Double-check _wsServer is still valid (defensive)
    if (!_wsServer) {
        LOG_WARN_COMPONENT("WebSocketServer", "WebSocket server pointer became null before send");
        return;
    }
    
    try {
        // CRITICAL: Check return value - sendTXT returns false on failure
        bool success = _wsServer->sendTXT(clientId, message.c_str(), message.length());
        if (success) {
            LOG_DEBUGF_COMPONENT("WebSocketServer", "Sent message to client %d", clientId);
        } else {
            LOG_WARNF_COMPONENT("WebSocketServer", "Failed to send message to client %d - connection may be dead", clientId);
            // Library will handle cleanup of dead connections via heartbeat
            // Don't try to disconnect here - let heartbeat handle it to avoid race conditions
        }
    } catch (...) {
        LOG_ERRORF_COMPONENT("WebSocketServer", "Exception in sendTXT to client %d", clientId);
    }
}

void SRWebSocketServer::handleWebSocketEvent(uint8_t clientId, WStype_t type, uint8_t* payload, size_t length) {
    LOG_DEBUGF_COMPONENT("WebSocketServer", "WebSocket event: clientId=%d, type=%d, length=%d", clientId, type, length);

    switch (type) {
        case WStype_DISCONNECTED:
            // SIMPLIFIED: No manual counter - library handles it!
            LOG_DEBUGF_COMPONENT("WebSocketServer", "WebSocket client %d disconnected (total: %d)", 
                clientId, getConnectedClients());
            // Don't call any _wsServer methods here - library is cleaning up
            break;
            
        case WStype_CONNECTED:
            // SIMPLIFIED: No manual counter - library handles it!
            LOG_DEBUGF_COMPONENT("WebSocketServer", "WebSocket client %d connected (total: %d)", 
                clientId, getConnectedClients());
            
            // CRITICAL: Delay sending status update to avoid race conditions
            // The library might still be setting up the connection
            // sendToClient() will verify the client is connected first
            sendStatusUpdate(clientId);
            break;
            
        case WStype_TEXT:
            LOG_DEBUGF_COMPONENT("WebSocketServer", "WebSocket text message received from client %d", clientId);
            processMessage(clientId, String((char*)payload));
            break;
            
        case WStype_ERROR:
            LOG_ERRORF_COMPONENT("WebSocketServer", "WebSocket error for client %d - connection may be unstable", clientId);
            // Don't force disconnect here - let the library handle it via heartbeat/timeout
            // Forcing disconnect during error handling can cause race conditions
            break;
    }
}

void SRWebSocketServer::processMessage(uint8_t clientId, const String& message) {
    unsigned long startTime = micros();
    // LOG_DEBUGF("Received message from client %d: %s", clientId, message.c_str());
    LOG_DEBUGF_COMPONENT("WebSocketServer", "Received message from client %d: %d bytes", clientId, message.length());
    
    // Parse JSON message
    // static DynamicJsonDocument doc(1024);
    std::shared_ptr<DynamicJsonDocument> doc = std::make_shared<DynamicJsonDocument>(1024);
    if (!doc) {
        LOG_ERROR_COMPONENT("WebSocketServer", "Failed to allocate memory for JSON document");
        sendToClient(clientId, "{\"error\":\"Failed to allocate memory for JSON document\"}");
        return;
    }
    DeserializationError error = deserializeJson(*doc, message);
    
    if (error) {
        LOG_ERRORF_COMPONENT("WebSocketServer", "JSON parse failed: %s", error.c_str());
        sendToClient(clientId, "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    JsonObject root = doc->as<JsonObject>();

    LOG_DEBUGF_COMPONENT("WebSocketServer", "Took %lu us to parse JSON", micros() - startTime);
    
    // Handle different command types - support both "type" and "t" (compact format)
    String type;
    if (root.containsKey("type")) {
        type = root["type"].as<String>();
    } else if (root.containsKey("t")) {
        type = root["t"].as<String>();
    } else {
        LOG_WARN_COMPONENT("WebSocketServer", "Message missing 'type' or 't' field");
        sendToClient(clientId, "{\"error\":\"Missing 'type' or 't' field\"}");
        return;
    }
    
    if (type == "effect") {
        // Route to command handler
        if (_commandHandler) {
            LOG_DEBUGF_COMPONENT("WebSocketServer", "Command handler supports queuing: %s", _commandHandler->supportsQueuing() ? "true" : "false");
            // Use queued command if handler supports it (for thread-safe processing)
            if (_commandHandler->supportsQueuing()) {
                LOG_DEBUGF_COMPONENT("WebSocketServer", "Handling queued command");
                _commandHandler->handleQueuedCommand(doc);
            } else {
                // Direct command handling for handlers that don't support queuing
                _commandHandler->handleCommand(root);
            }
        }
    } else if (type == "brightness") {
        handleBrightnessCommand(root);
    } else if (type == "next_effect") {
        // Cycle to next effect via PatternManager
        TriggerNextEffect();
        sendToClient(clientId, "{\"status\":\"next_effect_triggered\"}");
    } else if (type == "status") {
        handleStatusCommand(clientId);
    } else if (type == "trigger_choreography") {
        TriggerChoreography();
        sendToClient(clientId, "{\"status\":\"choreography_triggered\"}");
    } else {
        LOG_WARNF_COMPONENT("WebSocketServer", "Unknown command type: %s", type.c_str());
        sendToClient(clientId, "{\"error\":\"Unknown command type\"}");
    }
    
    unsigned long endTime = micros();
    LOG_DEBUGF_COMPONENT("WebSocketServer", "WebSocket command processed in %lu us", endTime - startTime);
    SaveUserPreferences(deviceState);
}

void SRWebSocketServer::processLEDCommand(const JsonObject& doc) {
    if (!_commandHandler) return;
    _commandHandler->handleCommand(doc);
}

void SRWebSocketServer::sendStatusUpdate(uint8_t clientId) {
    String statusJSON = generateStatusJSON();
    sendToClient(clientId, statusJSON);
}

void SRWebSocketServer::handleEffectCommand(const JsonObject& command) {
    if (!_commandHandler) return;
    processLEDCommand(command);
    LOG_DEBUGF_COMPONENT("WebSocketServer", "Processed effect command: %s", command["type"].as<String>().c_str());
}

void SRWebSocketServer::handleBrightnessCommand(const JsonObject& command) {
    if (!_commandHandler) return;
    
    if (command.containsKey("brightness")) {
        int brightness = command["brightness"];
        // Route brightness command through the handler interface
        // LEDManager will handle it appropriately, other handlers can ignore or handle as needed
        DynamicJsonDocument brightnessCmd(64);
        brightnessCmd["type"] = "brightness";
        brightnessCmd["brightness"] = brightness;
        _commandHandler->handleCommand(brightnessCmd.as<JsonObject>());
        
        // Update the brightness controller (LEDManager-specific, but safe to call)
        BrightnessController* brightnessController = BrightnessController::getInstance();
        if (brightnessController) {
            brightnessController->setBrightness(brightness);
        }
        LOG_DEBUGF_COMPONENT("WebSocketServer", "Set brightness to %d", brightness);
    }
}

void SRWebSocketServer::handleStatusCommand(uint8_t clientId) {
    sendStatusUpdate(clientId);
}

String SRWebSocketServer::generateStatusJSON() const {
    DynamicJsonDocument doc(512);
    
    doc["type"] = "status";
    doc["timestamp"] = millis();
    doc["connected_clients"] = getConnectedClients();  // Query library directly
    doc["server_status"] = _isRunning ? "running" : "stopped";
    doc["device_name"] = DeviceInfo::getDeviceName();
    
    // Add handler status if available
    if (_commandHandler) {
        doc["handler_status"] = _commandHandler->getStatus();
        int brightness = _commandHandler->getBrightness();
        if (brightness >= 0) {
            doc["brightness"] = brightness;
        }
    } else {
        doc["handler_status"] = "unavailable";
    }
    
    String result;
    serializeJson(doc, result);
    return result;
}
