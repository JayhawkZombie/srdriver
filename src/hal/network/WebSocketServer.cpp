#include "WebSocketServer.h"
#include "lights/LEDManager.h"
#include "freertos/LogManager.h"
#include <WebSocketsServer.h>

SRWebSocketServer::SRWebSocketServer(LEDManager* ledManager, uint16_t port) 
    : _ledManager(ledManager), _port(port), _isRunning(false), _connectedClients(0), _lastStatusUpdate(0) {
    _wsServer = nullptr;
}

SRWebSocketServer::~SRWebSocketServer() {
    stop();
}

void SRWebSocketServer::start() {
    LOG_DEBUG("SRWebSocketServer::start() called");
    
    if (_isRunning) {
        LOG_WARN("WebSocket server already running");
        return;
    }
    
    LOG_DEBUGF("SRWebSocketServer: Creating WebSocketsServer on port %d", _port);
    try {
        _wsServer = new WebSocketsServer(_port);
        LOG_DEBUG("SRWebSocketServer: WebSocketsServer instance created");
        
        LOG_DEBUG("SRWebSocketServer: Setting up event handler...");
        _wsServer->onEvent([this](uint8_t clientId, WStype_t type, uint8_t* payload, size_t length) {
            this->handleWebSocketEvent(clientId, type, payload, length);
        });
        LOG_DEBUG("SRWebSocketServer: Event handler set");
        
        LOG_DEBUG("SRWebSocketServer: Starting server...");
        _wsServer->begin();
        LOG_DEBUG("SRWebSocketServer: Server started");
        
        _isRunning = true;
        _lastStatusUpdate = 0;
        
        LOG_INFOF("SRWebSocketServer: WebSocket server started on port %d", _port);
    } catch (const std::exception& e) {
        LOG_ERRORF("SRWebSocketServer: Exception in start(): %s", e.what());
        if (_wsServer) {
            delete _wsServer;
            _wsServer = nullptr;
        }
        throw;
    } catch (...) {
        LOG_ERROR("SRWebSocketServer: Unknown exception in start()");
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
    
    LOG_INFO("WebSocket server stopped");
}

void SRWebSocketServer::update() {
    if (!_isRunning || !_wsServer) return;
    
    // Let WebSocketsServer handle its internal processing
    _wsServer->loop();
    
    // Handle periodic status updates (every 5 seconds)
    unsigned long now = millis();
    if (now - _lastStatusUpdate > 5000) {
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
    if (!_isRunning || _connectedClients == 0) return;
    
    String statusJSON = generateStatusJSON();
    broadcastMessage(statusJSON);
}

void SRWebSocketServer::broadcastMessage(const String& message) {
    if (!_isRunning || !_wsServer || _connectedClients == 0) return;
    
    _wsServer->broadcastTXT(message.c_str(), message.length());
    LOG_DEBUGF("Broadcasted message to %d clients: %s", _connectedClients, message.c_str());
}

void SRWebSocketServer::sendToClient(uint8_t clientId, const String& message) {
    if (!_isRunning || !_wsServer) return;
    
    _wsServer->sendTXT(clientId, message.c_str(), message.length());
    LOG_DEBUGF("Sent message to client %d: %s", clientId, message.c_str());
}

void SRWebSocketServer::handleWebSocketEvent(uint8_t clientId, WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            _connectedClients--;
            LOG_DEBUGF("WebSocket client %d disconnected", clientId);
            break;
            
        case WStype_CONNECTED:
            _connectedClients++;
            LOG_DEBUGF("WebSocket client %d connected", clientId);
            sendStatusUpdate(clientId); // Send current status
            break;
            
        case WStype_TEXT:
            processMessage(clientId, String((char*)payload));
            break;
            
        case WStype_ERROR:
            LOG_ERRORF("WebSocket error for client %d", clientId);
            break;
    }
}

void SRWebSocketServer::processMessage(uint8_t clientId, const String& message) {
    LOG_DEBUGF("Received message from client %d: %s", clientId, message.c_str());
    
    // Parse JSON message
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        LOG_ERRORF("JSON parse failed: %s", error.c_str());
        sendToClient(clientId, "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    JsonObject root = doc.as<JsonObject>();
    
    // Handle different command types
    if (root.containsKey("type")) {
        String type = root["type"];
        
        if (type == "effect") {
            handleEffectCommand(root);
        } else if (type == "brightness") {
            handleBrightnessCommand(root);
        } else if (type == "status") {
            handleStatusCommand(clientId);
        } else {
            LOG_WARNF("Unknown command type: %s", type.c_str());
            sendToClient(clientId, "{\"error\":\"Unknown command type\"}");
        }
    } else {
        LOG_WARN("Message missing 'type' field");
        sendToClient(clientId, "{\"error\":\"Missing 'type' field\"}");
    }
}

void SRWebSocketServer::processLEDCommand(const String& jsonCommand) {
    if (!_ledManager) return;
    
    // Parse JSON and route to LEDManager
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, jsonCommand);
    
    if (error) {
        LOG_ERRORF("JSON parse failed in processLEDCommand: %s", error.c_str());
        return;
    }
    
    JsonObject command = doc.as<JsonObject>();
    _ledManager->handleCommand(command);
}

void SRWebSocketServer::sendStatusUpdate(uint8_t clientId) {
    String statusJSON = generateStatusJSON();
    sendToClient(clientId, statusJSON);
}

void SRWebSocketServer::handleEffectCommand(const JsonObject& command) {
    if (!_ledManager) return;
    
    // Convert to JSON string and route to LEDManager
    String jsonCommand;
    serializeJson(command, jsonCommand);
    processLEDCommand(jsonCommand);
    
    LOG_DEBUGF("Processed effect command: %s", jsonCommand.c_str());
}

void SRWebSocketServer::handleBrightnessCommand(const JsonObject& command) {
    if (!_ledManager) return;
    
    if (command.containsKey("brightness")) {
        int brightness = command["brightness"];
        _ledManager->setBrightness(brightness);
        LOG_DEBUGF("Set brightness to %d", brightness);
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
    
    // Add LED status if LEDManager is available
    if (_ledManager) {
        doc["led_status"] = "active";
        doc["brightness"] = _ledManager->getBrightness();
    } else {
        doc["led_status"] = "unavailable";
    }
    
    String result;
    serializeJson(doc, result);
    return result;
}
