#pragma once

#include "SRTask.h"
#include "LogManager.h"
#include <WiFi.h>
#include "hal/network/WebSocketServer.h"

// Forward declaration
class BLEManager;

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
        // This will be called after preferences are loaded
        LOG_DEBUGF("WiFiManager: checkSavedCredentials() called - SSID length: %d, Password length: %d", _ssid.length(), _password.length());
        if (_ssid.length() > 0 && _password.length() > 0) {
            LOG_DEBUGF("WiFiManager: Found saved credentials for '%s', attempting connection", _ssid.c_str());
            _shouldConnect = true;
            _connectionAttempts = 0;
        } else {
            LOG_DEBUG("WiFiManager: No saved credentials found - skipping auto-connect");
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
        LOG_DEBUGF("WiFiManager: Credentials set for '%s', will attempt connection", ssid.c_str());
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

protected:
    /**
     * Main task loop - handles WiFi connection and status updates
     */
    void run() override {
        LOG_INFO("WiFiManager task started");
        LOG_PRINTF("Update interval: %d ms", _updateIntervalMs);
        
        TickType_t lastWakeTime = xTaskGetTickCount();
        
        while (true) {
            
            // Handle WiFi connection if needed (only if we have credentials)
            if (_shouldConnect && !isConnected() && _ssid.length() > 0) {
                attemptConnection();
            }
        
        // Update BLE status if connected
        if (isConnected() && _bleManager) {
            updateBLEStatus();
            
            // Start WebSocket server if not already started
            if (!_webSocketServer) {
                LOG_INFO("WiFiManager: WiFi connected, attempting to start WebSocket server...");
                try {
                    startWebSocketServer();
                    LOG_INFO("WiFiManager: WebSocket server started successfully");
                } catch (const std::exception& e) {
                    LOG_ERRORF("WiFiManager: WebSocket server failed to start: %s", e.what());
                } catch (...) {
                    LOG_ERROR("WiFiManager: WebSocket server failed to start (unknown error)");
                }
            }
        } else {
            // LOG_DEBUGF("WiFiManager: Not updating BLE status - isConnected: %s, _bleManager: %p", 
            //           isConnected() ? "true" : "false", _bleManager);
        }
        
        // Handle WebSocket server if connected
        if (isConnected() && _webSocketServer) {
            _webSocketServer->update(); // Just tick it!
        }
            
            // Increment update counter
            _updateCount++;
            
            // Log WiFi status every 10 seconds
            uint32_t now = millis();
            if (now - _lastStatusLog > 10000) {
                // LOG_DEBUGF("WiFi Update - Cycles: %d, Status: %s, IP: %s", 
                //           _updateCount, getStatus().c_str(), getIPAddress().c_str());
                _updateCount = 0;
                _lastStatusLog = now;
            }
            
            // Sleep until next update
            SRTask::sleepUntil(&lastWakeTime, _updateIntervalMs);
        }
    }

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
    
    // Connection management
    uint32_t _connectionAttempts;
    uint32_t _maxConnectionAttempts;
    uint32_t _connectionTimeoutMs;
    uint32_t _connectionStartTime;
    
    /**
     * Attempt WiFi connection
     */
    void attemptConnection() {
        if (_connectionAttempts >= _maxConnectionAttempts) {
            LOG_ERROR("WiFiManager: Max connection attempts reached, giving up");
            _shouldConnect = false;
            return;
        }
        
        if (_connectionAttempts == 0) {
            LOG_DEBUGF("WiFiManager: Starting connection to '%s' (attempt %d/%d)", 
                  _ssid.c_str(), _connectionAttempts + 1, _maxConnectionAttempts);
            _connectionStartTime = millis();
            WiFi.begin(_ssid.c_str(), _password.c_str());
        } else {
            // Log connection attempt progress
            LOG_DEBUGF("WiFiManager: Connection attempt %d/%d, status: %d", 
                      _connectionAttempts, _maxConnectionAttempts, WiFi.status());
        }
        
        _connectionAttempts++;
        
        // Check if connection succeeded
        LOG_DEBUGF("WiFiManager: Checking connection status: %d", WiFi.status());
        if (WiFi.status() == WL_CONNECTED) {
            String ip = getIPAddress();
            LOG_INFOF("WiFiManager: Connected to '%s' with IP: %s", 
                     _ssid.c_str(), ip.c_str());
            _shouldConnect = false;
            _connectionAttempts = 0;
            
            // Start WebSocket server on successful connection
            LOG_INFO("WiFiManager: Attempting to start WebSocket server...");
            try {
                startWebSocketServer();
                LOG_INFO("WiFiManager: WebSocket server started successfully");
            } catch (const std::exception& e) {
                LOG_ERRORF("WiFiManager: WebSocket server failed to start: %s", e.what());
            } catch (...) {
                LOG_ERROR("WiFiManager: WebSocket server failed to start (unknown error)");
            }
            
            // Update BLE characteristics immediately
            updateBLEStatus();
        } else if (millis() - _connectionStartTime > _connectionTimeoutMs) {
            LOG_ERRORF("WiFiManager: Connection timeout after %d attempts", _connectionAttempts);
            _shouldConnect = false;
            _connectionAttempts = 0;
            
            // Stop WebSocket server on connection failure
            stopWebSocketServer();
            
            // Update BLE status to failed
            updateBLEStatus();
        }
    }
    
    /**
     * Update BLE characteristics with WiFi status
     */
    void updateBLEStatus();
};
