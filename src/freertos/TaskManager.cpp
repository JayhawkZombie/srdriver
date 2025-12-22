#include "TaskManager.h"
#include "SystemMonitorTask.h"
#include "OLEDDisplayTask.h"
#include "WiFiManager.h"
#include "BLEUpdateTask.h"
// #include "LEDUpdateTask.h"
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

bool TaskManager::createOLEDDisplayTask(const JsonSettings* settings, uint32_t updateIntervalMs) {
    if (_oledDisplayTask != nullptr) {
        LOG_WARN_COMPONENT("TaskManager", "OLED display task already created");
        return _oledDisplayTask->isRunning();
    }

    _oledDisplayTask = new OLEDDisplayTask(settings, updateIntervalMs);
    if (_oledDisplayTask->start()) {
        LOG_INFO_COMPONENT("TaskManager", "OLED display task created and started");
        return true;
    } else {
        LOG_ERROR_COMPONENT("TaskManager", "Failed to start OLED display task");
        delete _oledDisplayTask;
        _oledDisplayTask = nullptr;
        return false;
    }
}

bool TaskManager::createWiFiManager(uint32_t updateIntervalMs) {
    if (_wifiManager != nullptr) {
        LOG_WARN_COMPONENT("TaskManager", "WiFi manager already created");
        return _wifiManager->isRunning();
    }
    
    _wifiManager = new WiFiManager(updateIntervalMs);
    if (_wifiManager->start()) {
        LOG_INFO_COMPONENT("TaskManager", "WiFi manager created and started");
        return true;
    } else {
        LOG_ERROR_COMPONENT("TaskManager", "Failed to start WiFi manager");
        delete _wifiManager;
        _wifiManager = nullptr;
        return false;
    }
}

bool TaskManager::createBLETask(BLEManager& manager, uint32_t updateIntervalMs) {
#if SUPPORTS_BLE
    if (_bleTask != nullptr) {
        LOG_WARN_COMPONENT("TaskManager", "BLE task already created");
        return _bleTask->isRunning();
    }
    
    _bleTask = new BLEUpdateTask(manager, updateIntervalMs);
    if (_bleTask->start()) {
        LOG_INFO_COMPONENT("TaskManager", "BLE task created and started");
        return true;
    } else {
        LOG_ERROR_COMPONENT("TaskManager", "Failed to start BLE task");
        delete _bleTask;
        _bleTask = nullptr;
        return false;
    }
#else
    LOG_INFO_COMPONENT("TaskManager", "BLE not supported on this platform");
    return false;
#endif
}

void TaskManager::cleanupAll() {
    cleanupSystemMonitorTask();
    cleanupOLEDDisplayTask();
    cleanupWiFiManager();
    cleanupBLETask();
    cleanupLEDTask();
}

void TaskManager::cleanupSystemMonitorTask() {
    if (_systemMonitorTask) {
        _systemMonitorTask->stop();
        delete _systemMonitorTask;
        _systemMonitorTask = nullptr;
        LOG_INFO_COMPONENT("TaskManager", "System monitor task cleaned up");
    }
}

void TaskManager::cleanupOLEDDisplayTask() {
    if (_oledDisplayTask) {
        _oledDisplayTask->stop();
        delete _oledDisplayTask;
        _oledDisplayTask = nullptr;
        LOG_INFO_COMPONENT("TaskManager", "OLED display task cleaned up");
    }
}

bool TaskManager::isSystemMonitorTaskRunning() const {
    return _systemMonitorTask != nullptr && _systemMonitorTask->isRunning();
}

bool TaskManager::isOLEDDisplayTaskRunning() const {
    return _oledDisplayTask != nullptr && _oledDisplayTask->isRunning();
}

void TaskManager::cleanupWiFiManager() {
    if (_wifiManager) {
        _wifiManager->stop();
        delete _wifiManager;
        _wifiManager = nullptr;
        LOG_INFO_COMPONENT("TaskManager", "WiFi manager cleaned up");
    }
}

bool TaskManager::isWiFiManagerRunning() const {
    return _wifiManager != nullptr && _wifiManager->isRunning();
}

void TaskManager::cleanupBLETask() {
#if SUPPORTS_BLE
    if (_bleTask) {
        _bleTask->stop();
        delete _bleTask;
        _bleTask = nullptr;
        LOG_INFO_COMPONENT("TaskManager", "BLE task cleaned up");
    }
#endif
}

bool TaskManager::isBLETaskRunning() const {
#if SUPPORTS_BLE
    return _bleTask != nullptr && _bleTask->isRunning();
#else
    return false;
#endif
}
