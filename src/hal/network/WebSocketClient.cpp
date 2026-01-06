#include "WebSocketClient.h"
#include <ArduinoJson.h>
#include <WiFi.h>

// Static instance pointer for event callback
SRWebSocketClient* SRWebSocketClient::_eventInstance = nullptr;

SRWebSocketClient::SRWebSocketClient(const String& ipAddress)
    : _ipAddress(ipAddress),
      _client(nullptr),
      _state(DISCONNECTED),
      _autoReconnect(true),
      _lastDisconnectTime(0),
      _lastActivity(0),
      _reconnectAttempts(0),
      _nextReconnectDelay(RECONNECT_DELAY_MS) {
    _lastStatusMessage = "";
}

SRWebSocketClient::~SRWebSocketClient() {
    disconnect();
    if (_client) {
        delete _client;
        _client = nullptr;
    }
}

bool SRWebSocketClient::connect() {
    if (_state == CONNECTED || _state == CONNECTING) {
        LOG_WARNF_COMPONENT("WebSocketClient", "Already connected or connecting to %s", _ipAddress.c_str());
        return _state == CONNECTED;
    }
    
    LOG_INFOF_COMPONENT("WebSocketClient", "Connecting to %s:%d", _ipAddress.c_str(), WS_CLIENT_PORT);
    
    // Create client if needed
    if (!_client) {
        _client = new WebSocketsClient();
        if (!_client) {
            LOG_ERROR_COMPONENT("WebSocketClient", "Failed to allocate WebSocketsClient");
            return false;
        }
        
        // Set up event handler
        // _eventInstance = this;  // Set static pointer for callback
        // _client->onEvent(webSocketEvent);
        _client->onEvent([this](WStype_t type, uint8_t* payload, size_t length) {
            this->handleEvent(type, payload, length);
        });
    }
    
    _state = CONNECTING;
    
    LOG_DEBUGF_COMPONENT("WebSocketClient", "Connecting to %s:%d", _ipAddress.c_str(), WS_CLIENT_PORT);
    
    // Begin connection - WebSocketsClient::begin() takes host, port, url, protocol
    _client->begin(_ipAddress.c_str(), WS_CLIENT_PORT, "/", "arduino");
    _client->setReconnectInterval(0);  // We handle reconnection ourselves
    
    // Connection is asynchronous, no need to delay here
    // The connection state will be updated via the event callback
    
    return true;
}

void SRWebSocketClient::disconnect() {
    if (_client && (_state == CONNECTED || _state == CONNECTING)) {
        LOG_INFOF_COMPONENT("WebSocketClient", "Disconnecting from %s", _ipAddress.c_str());
        _client->disconnect();
    }
    
    _state = DISCONNECTED;
    _lastDisconnectTime = millis();
}

bool SRWebSocketClient::isConnected() const {
    if (!_client) return false;
    
    // Trust the WebSocket library's connection state
    // This is the source of truth since connection is asynchronous
    return _client->isConnected();
}

SRWebSocketClient::ConnectionState SRWebSocketClient::getState() const {
    return _state;
}

bool SRWebSocketClient::sendCommand(const String& jsonCommand) {
    if (!isConnected() || !_client) {
        LOG_WARNF_COMPONENT("WebSocketClient", "Cannot send command to %s: not connected", _ipAddress.c_str());
        return false;
    }
    
    LOG_DEBUGF_COMPONENT("WebSocketClient", "Sending command to %s: %s", _ipAddress.c_str(), jsonCommand.c_str());
    
    // sendTXT() expects non-const String&, so we need to make a copy
    String cmdCopy = jsonCommand;
    bool result = _client->sendTXT(cmdCopy);
    if (result) {
        _lastActivity = millis();
    }
    
    return result;
}

bool SRWebSocketClient::sendBrightness(uint8_t brightness) {
    // Create JSON command
    StaticJsonDocument<64> doc;
    doc["t"] = "brightness";
    doc["brightness"] = brightness;
    
    String jsonCommand;
    serializeJson(doc, jsonCommand);
    
    return sendCommand(jsonCommand);
}

void SRWebSocketClient::resetReconnectAttempts() {
    _reconnectAttempts = 0;
    _nextReconnectDelay = RECONNECT_DELAY_MS;
    LOG_INFOF_COMPONENT("WebSocketClient", "Reset reconnect attempts for %s", _ipAddress.c_str());
}

void SRWebSocketClient::update() {
    if (!_client) return;
    
    // Let WebSocketsClient handle its internal processing
    _client->loop();
    
    // Check for auto-reconnect
    if (_state == DISCONNECTED || _state == RECONNECTING) {
        if (shouldAutoReconnect()) {
            tryReconnect();
        }
    }
}

bool SRWebSocketClient::shouldAutoReconnect() const {
    if (!_autoReconnect) return false;
    if (_reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) return false;
    if (_state == CONNECTING || _state == CONNECTED) return false;
    
    unsigned long elapsed = millis() - _lastDisconnectTime;
    return elapsed >= _nextReconnectDelay;
}

void SRWebSocketClient::tryReconnect() {
    if (!shouldAutoReconnect()) return;
    
    LOG_INFOF_COMPONENT("WebSocketClient", "Attempting to reconnect to %s (attempt %d/%d)", 
                       _ipAddress.c_str(), _reconnectAttempts + 1, MAX_RECONNECT_ATTEMPTS);
    
    _state = RECONNECTING;
    
    if (connect()) {
        // Connection initiated, wait for callback to confirm
        // State will be updated in handleEvent()
    } else {
        _reconnectAttempts++;
        calculateNextReconnectDelay();
        _state = DISCONNECTED;
        _lastDisconnectTime = millis();
    }
}

void SRWebSocketClient::calculateNextReconnectDelay() {
    // Exponential backoff: base * (multiplier ^ attempts)
    float baseDelay = RECONNECT_DELAY_MS;
    float multiplier = RECONNECT_BACKOFF_MULTIPLIER;
    float delay = baseDelay * pow(multiplier, _reconnectAttempts);
    
    // Add random jitter (±500ms)
    int jitter = random(-RECONNECT_JITTER_MS / 2, RECONNECT_JITTER_MS / 2);
    delay += jitter;
    
    // Clamp to reasonable maximum (5 minutes)
    if (delay > 300000) {
        delay = 300000;
    }
    
    _nextReconnectDelay = (unsigned long)delay;
    
    LOG_DEBUGF_COMPONENT("WebSocketClient", "Next reconnect delay for %s: %lu ms (attempt %d)", 
                         _ipAddress.c_str(), _nextReconnectDelay, _reconnectAttempts);
}

// Static event handler wrapper
void SRWebSocketClient::webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    if (_eventInstance) {
        _eventInstance->handleEvent(type, payload, length);
    }
}

// Instance event handler
void SRWebSocketClient::handleEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            {
                String reason = length > 0 ? String((char*)payload) : "Unknown";
                if (_state == CONNECTING) {
                    LOG_ERRORF_COMPONENT("WebSocketClient", "Connection to %s failed: %s", _ipAddress.c_str(), reason.c_str());
                    _autoReconnect = false;
                } else {
                    LOG_INFOF_COMPONENT("WebSocketClient", "Disconnected from %s: %s", _ipAddress.c_str(), reason.c_str());
                    _autoReconnect = false;
                }
            }
            _state = DISCONNECTED;
            _lastDisconnectTime = millis();
            if (_reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
                calculateNextReconnectDelay();
            }
            break;
            
        case WStype_CONNECTED:
            LOG_INFOF_COMPONENT("WebSocketClient", "Connected to %s", _ipAddress.c_str());
            _state = CONNECTED;
            _reconnectAttempts = 0;  // Reset on successful connection
            _nextReconnectDelay = RECONNECT_DELAY_MS;
            _lastActivity = millis();
            break;
            
        case WStype_TEXT:
            {
                String message = String((char*)payload);
                LOG_DEBUGF_COMPONENT("WebSocketClient", "Received message from %s: %s", 
                                     _ipAddress.c_str(), message.c_str());
                _lastStatusMessage = message;
                _lastActivity = millis();
            }
            break;
            
        case WStype_ERROR:
            {
                String errorMsg = length > 0 ? String((char*)payload) : "Unknown error";
                LOG_ERRORF_COMPONENT("WebSocketClient", "Error from %s: %s", 
                                    _ipAddress.c_str(), errorMsg.c_str());
                // If we get an error while connecting, mark as disconnected
                if (_state == CONNECTING) {
                    _state = DISCONNECTED;
                    _lastDisconnectTime = millis();
                    if (_reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
                        calculateNextReconnectDelay();
                    }
                }
            }
            break;
            
        case WStype_PING:
            LOG_DEBUGF_COMPONENT("WebSocketClient", "Ping from %s", _ipAddress.c_str());
            break;
            
        case WStype_PONG:
            LOG_DEBUGF_COMPONENT("WebSocketClient", "Pong from %s", _ipAddress.c_str());
            _lastActivity = millis();
            break;
            
        default:
            LOG_DEBUGF_COMPONENT("WebSocketClient", "Event type %d from %s", type, _ipAddress.c_str());
            break;
    }
}

