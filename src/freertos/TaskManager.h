#pragma once

#include "PlatformConfig.h"
#include "SRTask.h"
#include "../config/JsonSettings.h"

#include <memory>

// Forward declarations
class SystemMonitorTask;
class OLEDDisplayTask;
class WiFiManager;
class BLEUpdateTask;
class BLEManager;
class LEDUpdateTask;
class LVGLDisplayTask;

/**
 * TaskManager - Singleton for managing all FreeRTOS tasks
 * 
 * Provides centralized creation, access, and cleanup of all system tasks.
 * Replaces global task pointers with accessor methods.
 * 
 * Currently supports: SystemMonitorTask
 * Additional tasks will be added incrementally.
 */
class TaskManager {
public:
    static TaskManager& getInstance();
    
    // Factory methods - return true if task created and started successfully
    bool createSystemMonitorTask(uint32_t updateIntervalMs = 1000);
    bool createOLEDDisplayTask(const JsonSettings* settings = nullptr, uint32_t updateIntervalMs = 200);
    bool createWiFiManager(uint32_t updateIntervalMs = 10);
    bool createBLETask(BLEManager& manager, uint32_t updateIntervalMs = 10);
    bool createLEDTask(uint32_t updateIntervalMs = 16);  // Default 60 FPS
    bool createLVGLDisplayTask(const JsonSettings* settings = nullptr, uint32_t updateIntervalMs = 200);
    
    // Accessors - return nullptr if task not created
    SystemMonitorTask* getSystemMonitorTask() const { return _systemMonitorTask; }
    OLEDDisplayTask* getOLEDDisplayTask() const { return _oledDisplayTask; }
    WiFiManager* getWiFiManager() const { return _wifiManager; }
    BLEUpdateTask* getBLETask() const { return _bleTask; }
    LEDUpdateTask* getLEDTask() const { return _ledTask; }
    LVGLDisplayTask* getLVGLDisplayTask() const { return _lvglDisplayTask; }
    
    // Cleanup
    void cleanupAll();
    void cleanupSystemMonitorTask();
    void cleanupOLEDDisplayTask();
    void cleanupWiFiManager();
    void cleanupBLETask();
    void cleanupLEDTask();
    void cleanupLVGLDisplayTask();
    
    // Check if tasks are running
    bool isSystemMonitorTaskRunning() const;
    bool isOLEDDisplayTaskRunning() const;
    bool isWiFiManagerRunning() const;
    bool isBLETaskRunning() const;
    bool isLEDTaskRunning() const;
    bool isLVGLDisplayTaskRunning() const;
private:
    TaskManager() = default;
    ~TaskManager() { cleanupAll(); }
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;
    
    // Task pointers
    SystemMonitorTask *_systemMonitorTask = nullptr;
    OLEDDisplayTask *_oledDisplayTask = nullptr;
    WiFiManager *_wifiManager = nullptr;
    BLEUpdateTask *_bleTask = nullptr;
    LEDUpdateTask *_ledTask = nullptr;
    LVGLDisplayTask *_lvglDisplayTask = nullptr;
};

