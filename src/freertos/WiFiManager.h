#pragma once

#include "SRTask.h"
#include "LogManager.h"
#include <WiFi.h>
#include "hal/network/WebSocketServer.h"
#include <vector>

// Forward declaration
class BLEManager;

struct NetworkCredentials {
    String ssid;
    String password;
};

/**
 * WiFiManager - FreeRTOS task for WiFi management
 * 
 * Handles:
 * - WiFi connection management
 * - WiFi status updates
 * - IP address management
 * - BLE integration for WiFi status
 */
class WiFiManager : public SRTask {
public:
    WiFiManager(uint32_t updateIntervalMs = 10,  // 10ms for WiFi
                uint32_t stackSize = 8192,
                UBaseType_t priority = tskIDLE_PRIORITY + 1,
                BaseType_t core = 0)  // Pin to core 0 (WiFi/BLE core)
        : SRTask("WiFiManager", stackSize, priority, core),
          _updateIntervalMs(updateIntervalMs),
          _updateCount(0),
          _lastStatusLog(0),
          _bleManager(nullptr),
          _connectionAttempts(0),
          _maxConnectionAttempts(10),
          _connectionTimeoutMs(10000) {}
    
    /**
     * Set BLE manager for status updates
     */
    void setBLEManager(BLEManager* manager) {
        _bleManager = manager;
    }
    
    /**
     * Set LED manager for WebSocket command routing
     */
    void setLEDManager(LEDManager* ledManager) {
        _ledManager = ledManager;
    }
    
    /**
     * Check for saved credentials and attempt connection
     */
    void checkSavedCredentials() {
        if (_ssid.length() > 0 && _password.length() > 0) {
            _shouldConnect = true;
            _connectionAttempts = 0;
        }
    }
    
    /**
     * Set WiFi credentials and trigger connection
     */
    void setCredentials(const String& ssid, const String& password) {
        _ssid = ssid;
        _password = password;
        _shouldConnect = true;
        _connectionAttempts = 0; // Reset attempts for new credentials
        LOG_DEBUGF_COMPONENT("WiFiManager", "Credentials set for '%s', will attempt connection", ssid.c_str());
    }
    
    /**
     * Get current connection status
     */
    bool isConnected() const {
        wl_status_t status = WiFi.status();
        return status == WL_CONNECTED;
    }
    
    /**
     * Get current IP address
     */
    String getIPAddress() const {
        if (isConnected()) {
            IPAddress ip = WiFi.localIP();
            if (ip != INADDR_NONE) {
                return ip.toString();
            }
        }
        return "";
    }
    
    /**
     * Get current status string
     */
    String getStatus() const {
        switch (WiFi.status()) {
            case WL_CONNECTED:
                return "connected";
            case WL_NO_SSID_AVAIL:
                return "no_ssid";
            case WL_CONNECT_FAILED:
                return "connect_failed";
            case WL_CONNECTION_LOST:
                return "connection_lost";
            case WL_DISCONNECTED:
                return "disconnected";
            default:
                return "unknown";
        }
    }
    
    /**
     * Get update count
     */
    uint32_t getUpdateCount() const { return _updateCount; }
    
    /**
     * Set update interval
     */
    void setUpdateInterval(uint32_t intervalMs) {
        _updateIntervalMs = intervalMs;
    }
    
    /**
     * WebSocket server management
     */
    void startWebSocketServer();
    void stopWebSocketServer();
    bool isWebSocketServerRunning() const;
    void broadcastToClients(const String& message);
    void setKnownNetworks(const std::vector<NetworkCredentials>& knownNetworks) { _knownNetworks = knownNetworks; }

protected:
    /**
     * Main task loop - handles WiFi connection and status updates
     */
    void run() override;

private:
    BLEManager* _bleManager;
    LEDManager* _ledManager;
    SRWebSocketServer* _webSocketServer = nullptr;
    uint32_t _updateIntervalMs;
    uint32_t _updateCount;
    uint32_t _lastStatusLog;
    
    // WiFi credentials
    String _ssid;
    String _password;
    bool _shouldConnect;

    std::vector<NetworkCredentials> _knownNetworks;
    
    // Connection management
    uint32_t _connectionAttempts;
    uint32_t _maxConnectionAttempts;
    uint32_t _connectionTimeoutMs;
    uint32_t _connectionStartTime;
    
    /**
     * Attempt WiFi connection
     */
    void attemptConnection();
    
    /**
     * Update BLE characteristics with WiFi status
     */
    void updateBLEStatus();
};
