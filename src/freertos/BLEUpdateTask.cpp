#include "BLEUpdateTask.h"
#include "../hal/ble/BLEManager.h"

void BLEUpdateTask::run()
{
    LOG_INFO_COMPONENT("BLEUpdateTask", "BLE update task started");
    LOG_INFOF_COMPONENT("BLEUpdateTask", "Update interval: %d ms", _updateIntervalMs);

    TickType_t lastWakeTime = xTaskGetTickCount();

    while (true)
    {
        // Update BLE manager (this handles all BLE operations)
        _bleManager.update();

        // Increment update counter
        _updateCount++;

        // Log BLE status every 5 seconds
        uint32_t now = millis();
        if (now - _lastStatusLog > 5000)
        {
            LOG_DEBUGF_COMPONENT("BLEUpdateTask", "BLE Update - Cycles: %d, Interval: %d ms",
                _updateCount, _updateIntervalMs);
            _updateCount = 0;
            _lastStatusLog = now;
        }

        // Sleep until next update
        SRTask::sleepUntil(&lastWakeTime, _updateIntervalMs);
    }
}


bool BLEUpdateTask::isConnected() const {
    return _bleManager.isConnected();
}