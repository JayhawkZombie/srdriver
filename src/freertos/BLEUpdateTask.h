#pragma once

#include "SRTask.h"
#include "LogManager.h"

// Forward declaration
class BLEManager;

/**
 * BLEUpdateTask - FreeRTOS task for BLE management
 * 
 * Handles:
 * - BLE connection management
 * - BLE characteristic updates
 * - BLE command processing
 * - Non-blocking BLE operations
 */
class BLEUpdateTask : public SRTask {
public:
    BLEUpdateTask(BLEManager& manager,
                  uint32_t updateIntervalMs = 10,  // 10ms for BLE responsiveness
                  uint32_t stackSize = 8192,
                  UBaseType_t priority = tskIDLE_PRIORITY + 1,  // Medium priority
                  BaseType_t core = 0)  // Pin to core 0 (WiFi/BLE core)
        : SRTask("BLEUpdate", stackSize, priority, core),
          _bleManager(manager),
          _updateIntervalMs(updateIntervalMs),
          _updateCount(0),
          _lastStatusLog(0) {}
    
    /**
     * Get current update count
     */
    uint32_t getUpdateCount() const { return _updateCount; }
    
    /**
     * Get update interval
     */
    uint32_t getUpdateInterval() const { return _updateIntervalMs; }
    
    /**
     * Set update interval (for dynamic adjustment)
     */
    void setUpdateInterval(uint32_t intervalMs) {
        _updateIntervalMs = intervalMs;
    }

protected:
    /**
     * Main task loop - handles BLE updates
     */
    void run() override;
private:
    BLEManager& _bleManager;
    uint32_t _updateIntervalMs;
    uint32_t _updateCount;
    uint32_t _lastStatusLog;
}; 