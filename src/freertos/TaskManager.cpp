#include "TaskManager.h"
#include "SystemMonitorTask.h"
#include "LogManager.h"

TaskManager& TaskManager::getInstance() {
    static TaskManager instance;
    return instance;
}

bool TaskManager::createSystemMonitorTask(uint32_t updateIntervalMs) {
    if (_systemMonitorTask != nullptr) {
        LOG_WARN_COMPONENT("TaskManager", "System monitor task already created");
        return _systemMonitorTask->isRunning();
    }
    
    _systemMonitorTask = new SystemMonitorTask(updateIntervalMs);
    if (_systemMonitorTask->start()) {
        LOG_INFO_COMPONENT("TaskManager", "System monitor task created and started");
        return true;
    } else {
        LOG_ERROR_COMPONENT("TaskManager", "Failed to start system monitor task");
        delete _systemMonitorTask;
        _systemMonitorTask = nullptr;
        return false;
    }
}

void TaskManager::cleanupAll() {
    cleanupSystemMonitorTask();
}

void TaskManager::cleanupSystemMonitorTask() {
    if (_systemMonitorTask) {
        _systemMonitorTask->stop();
        delete _systemMonitorTask;
        _systemMonitorTask = nullptr;
        LOG_INFO_COMPONENT("TaskManager", "System monitor task cleaned up");
    }
}

bool TaskManager::isSystemMonitorTaskRunning() const {
    return _systemMonitorTask != nullptr && _systemMonitorTask->isRunning();
}

