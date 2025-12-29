#pragma once

#include <Arduino.h>
#include <WebSocketsClient.h>
#include "freertos/LogManager.h"

// Configuration constants
#define MAX_DEVICES 10                    // Maximum number of devices
#define RECONNECT_DELAY_MS 5000           // Initial reconnect delay (5 seconds)
#define MAX_RECONNECT_ATTEMPTS 10         // Stop after 10 failures
#define RECONNECT_BACKOFF_MULTIPLIER 2.0f  // Exponential backoff multiplier
#define RECONNECT_JITTER_MS 1000          // Random jitter range (±500ms)
#define WS_CLIENT_PORT 8080               // Default WebSocket port

/**
 * SRWebSocketClient - WebSocket client for connecting to remote SRDriver devices
 * 
 * Features:
 * - Connect/disconnect to remote device by IP address
 * - Send JSON commands (brightness, effects, etc.)
 * - Receive status updates from device
 * - Auto-reconnect with exponential backoff and jitter
 * - Connection state tracking
 */
class SRWebSocketClient {
public:
    enum ConnectionState {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        RECONNECTING
    };
    
    SRWebSocketClient(const String& ipAddress);
    ~SRWebSocketClient();
    
    // Connection management
    bool connect();
    void disconnect();
    bool isConnected() const;
    ConnectionState getState() const;
    
    // Command sending
    bool sendCommand(const String& jsonCommand);
    bool sendBrightness(uint8_t brightness);
    
    // Status
    String getIP() const { return _ipAddress; }
    String getLastStatus() const { return _lastStatusMessage; }
    unsigned long getLastActivity() const { return _lastActivity; }
    
    // Auto-reconnect
    void setAutoReconnect(bool enable) { _autoReconnect = enable; }
    bool getAutoReconnect() const { return _autoReconnect; }
    void resetReconnectAttempts();  // Manual trigger to retry
    
    // Must call in loop() to process events
    void update();
    
private:
    String _ipAddress;
    WebSocketsClient* _client;
    ConnectionState _state;
    bool _autoReconnect;
    unsigned long _lastDisconnectTime;
    unsigned long _lastActivity;
    String _lastStatusMessage;
    
    // Reconnect tracking
    uint8_t _reconnectAttempts;
    unsigned long _nextReconnectDelay;
    
    // Auto-reconnect logic (self-contained)
    bool shouldAutoReconnect() const;
    void tryReconnect();
    void calculateNextReconnectDelay();
    
    // WebSocket event handler (static wrapper)
    static void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    
    // Instance event handler
    void handleEvent(WStype_t type, uint8_t* payload, size_t length);
    
    // Static instance pointer for event callback
    static SRWebSocketClient* _eventInstance;
};

