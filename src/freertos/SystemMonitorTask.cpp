#include "SystemMonitorTask.h"
#include "freertos/task.h"

#if SUPPORTS_ESP32_APIS
#include "esp_task_wdt.h"
#include "esp_system.h"
#endif

// Helper function to get state string
const char* TaskStatsCollection::getStateString(eTaskState state) {
    switch (state) {
        case eRunning:  return "Running";
        case eReady:    return "Ready";
        case eBlocked:  return "Blocked";
        case eSuspended: return "Suspended";
        case eDeleted:  return "Deleted";
        case eInvalid:  return "Invalid";
        default:        return "Unknown";
    }
}

void SystemMonitorTask::updateTaskStats() {
    // For now, we'll use a simpler approach that doesn't require uxTaskGetSystemState
    // which isn't available in the default ESP32 Arduino framework build.
    // 
    // TODO: Enable configUSE_TRACE_FACILITY=1 in ESP-IDF configuration or use
    //       alternative task monitoring APIs
    
    // Get number of tasks
    UBaseType_t numTasks = uxTaskGetNumberOfTasks();
    
    if (numTasks == 0) {
        _lastTaskStats.tasks.clear();
        _lastTaskStats.totalTasks = 0;
        _lastTaskStats.lastUpdateTime = millis();
        return;
    }

    // uxTaskGetSystemState(NULL, 0, NULL);
    
    // For now, we'll create a minimal implementation that shows we have tasks
    // but without detailed information until uxTaskGetSystemState is available
    _taskStats.tasks.clear();
    _taskStats.totalTasks = numTasks;
    _taskStats.lastUpdateTime = millis();
    
    // Update last stats (thread-safe copy)
    _lastTaskStats = _taskStats;
    
    // Log a warning that detailed task info is not available
    static bool warned = false;
    if (!warned) {
        LOG_WARN_COMPONENT("SystemMonitor", "uxTaskGetSystemState not available - task details limited. Enable configUSE_TRACE_FACILITY=1 in ESP-IDF config.");
        warned = true;
    }
}
