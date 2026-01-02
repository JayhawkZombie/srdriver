#include "WebSocketServer.h"
#include "lights/LEDManager.h"
#include "freertos/LogManager.h"
#include "controllers/BrightnessController.h"
#include <WebSocketsServer.h>
#include "../PatternManager.h"
#include "DeviceState.h"

SRWebSocketServer::SRWebSocketServer(ICommandHandler* commandHandler, uint16_t port) 
    : _commandHandler(commandHandler), _port(port), _isRunning(false), _connectedClients(0), _lastStatusUpdate(0) {
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
        
        _isRunning = true;
        _lastStatusUpdate = 0;
        
        LOG_INFOF_COMPONENT("WebSocketServer", "SRWebSocketServer: WebSocket server started on port %d", _port);
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
    _connectedClients = 0;
    
    LOG_INFO_COMPONENT("WebSocketServer", "WebSocket server stopped");
}

void SRWebSocketServer::update() {
    // LOG_DEBUG_COMPONENT("WebSocketServer", "WebSocket update called");
    if (!_isRunning || !_wsServer) {
        LOG_DEBUGF_COMPONENT("WebSocketServer", "WebSocket update: not running (isRunning: %s, wsServer: %p)", 
                  _isRunning ? "true" : "false", _wsServer);
        return;
    }
    
    // Let WebSocketsServer handle its internal processing
    _wsServer->loop();
    
    // Handle periodic status updates (every 5 seconds)
    unsigned long now = millis();
    if (now - _lastStatusUpdate > 5000) {
        LOG_DEBUGF_COMPONENT("WebSocketServer", "WebSocket update: broadcasting status to %d clients", _connectedClients);
        broadcastStatus();
        _lastStatusUpdate = now;
    }
}

bool SRWebSocketServer::isRunning() const {
    return _isRunning;
}

uint8_t SRWebSocketServer::getConnectedClients() const {
    return _connectedClients;
}

String SRWebSocketServer::getStatus() const {
    if (!_isRunning) return "stopped";
    return String("running on port ") + String(_port) + " with " + String(_connectedClients) + " clients";
}

void SRWebSocketServer::broadcastStatus() {
    if (!_isRunning || _connectedClients == 0 || _connectedClients > 10) {
        // Sanity check: max 10 clients reasonable, 255 indicates corruption
        if (_connectedClients > 10) {
            LOG_ERRORF_COMPONENT("WebSocketServer", "Invalid client count: %d, resetting to 0", _connectedClients);
            _connectedClients = 0;
        }
        return;
    }
    
    String statusJSON = generateStatusJSON();
    broadcastMessage(statusJSON);
}

void SRWebSocketServer::broadcastMessage(const String& message) {
    if (!_isRunning || !_wsServer || _connectedClients == 0 || _connectedClients > 10) {
        if (_connectedClients > 10) {
            LOG_ERRORF_COMPONENT("WebSocketServer", "Invalid client count in broadcast: %d", _connectedClients);
            _connectedClients = 0;
        }
        return;
    }
    
    _wsServer->broadcastTXT(message.c_str(), message.length());
    LOG_DEBUGF_COMPONENT("WebSocketServer", "Broadcasted message to %d clients: %s", _connectedClients, message.c_str());
}

void SRWebSocketServer::sendToClient(uint8_t clientId, const String& message) {
    if (!_isRunning || !_wsServer) return;
    
    _wsServer->sendTXT(clientId, message.c_str(), message.length());
    LOG_DEBUGF_COMPONENT("WebSocketServer", "Sent message to client %d: %s", clientId, message.c_str());
}

void SRWebSocketServer::handleWebSocketEvent(uint8_t clientId, WStype_t type, uint8_t* payload, size_t length) {
    LOG_DEBUGF_COMPONENT("WebSocketServer", "WebSocket event: clientId=%d, type=%d, length=%d", clientId, type, length);

    switch (type) {
        case WStype_DISCONNECTED:
            if (_connectedClients > 0) {
            _connectedClients--;
            } else {
                LOG_WARN_COMPONENT("WebSocketServer", "Disconnect event but client count already 0");
            }
            LOG_DEBUGF_COMPONENT("WebSocketServer", "WebSocket client %d disconnected (total: %d)", clientId, _connectedClients);
            break;
            
        case WStype_CONNECTED:
            if (_connectedClients < 255) {  // Prevent overflow
            _connectedClients++;
            } else {
                LOG_WARN_COMPONENT("WebSocketServer", "Connect event but client count at max (255)");
            }
            LOG_DEBUGF_COMPONENT("WebSocketServer", "WebSocket client %d connected (total: %d)", clientId, _connectedClients);
            sendStatusUpdate(clientId); // Send current status
            break;
            
        case WStype_TEXT:
            LOG_DEBUGF_COMPONENT("WebSocketServer", "WebSocket text message received from client %d", clientId);
            processMessage(clientId, String((char*)payload));
            break;
            
        case WStype_ERROR:
            // LOG_ERRORF("WebSocket error for client %d", clientId);
            LOG_ERRORF_COMPONENT("WebSocketServer", "WebSocket error for client %d", clientId);
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
    doc["connected_clients"] = _connectedClients;
    doc["server_status"] = _isRunning ? "running" : "stopped";
    
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
