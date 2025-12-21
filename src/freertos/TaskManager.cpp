#include "TaskManager.h"
#include "SystemMonitorTask.h"
#include "OLEDDisplayTask.h"
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
    
    _systemMonitorTask = std::unique_ptr<SystemMonitorTask>(new SystemMonitorTask(updateIntervalMs));
    if (_systemMonitorTask->start()) {
        LOG_INFO_COMPONENT("TaskManager", "System monitor task created and started");
        return true;
    } else {
        LOG_ERROR_COMPONENT("TaskManager", "Failed to start system monitor task");
        _systemMonitorTask.reset();
        return false;
    }
}

bool TaskManager::createOLEDDisplayTask(const JsonSettings* settings, uint32_t updateIntervalMs) {
    if (_oledDisplayTask != nullptr) {
        LOG_WARN_COMPONENT("TaskManager", "OLED display task already created");
        return _oledDisplayTask->isRunning();
    }

    _oledDisplayTask = std::unique_ptr<OLEDDisplayTask>(new OLEDDisplayTask(settings, updateIntervalMs));
    if (_oledDisplayTask->start()) {
        LOG_INFO_COMPONENT("TaskManager", "OLED display task created and started");
        return true;
    } else {
        LOG_ERROR_COMPONENT("TaskManager", "Failed to start OLED display task");
        _oledDisplayTask.reset();
        return false;
    }
}

void TaskManager::cleanupAll() {
    cleanupSystemMonitorTask();
    cleanupOLEDDisplayTask();
}

void TaskManager::cleanupSystemMonitorTask() {
    if (_systemMonitorTask) {
        _systemMonitorTask->stop();
        _systemMonitorTask.reset();
        LOG_INFO_COMPONENT("TaskManager", "System monitor task cleaned up");
    }
}

void TaskManager::cleanupOLEDDisplayTask() {
    if (_oledDisplayTask) {
        _oledDisplayTask->stop();
        _oledDisplayTask.reset();
        LOG_INFO_COMPONENT("TaskManager", "OLED display task cleaned up");
    }
}

bool TaskManager::isSystemMonitorTaskRunning() const {
    return _systemMonitorTask != nullptr && _systemMonitorTask->isRunning();
}

bool TaskManager::isOLEDDisplayTaskRunning() const {
    return _oledDisplayTask != nullptr && _oledDisplayTask->isRunning();
}