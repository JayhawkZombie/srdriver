#pragma once

#include "PlatformConfig.h"
#include "SRTask.h"
#include "../config/JsonSettings.h"

#include <memory>

// Forward declarations
class SystemMonitorTask;
class OLEDDisplayTask;

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
    
    // Accessors - return nullptr if task not created
    SystemMonitorTask* getSystemMonitorTask() const { return _systemMonitorTask.get(); }
    OLEDDisplayTask* getOLEDDisplayTask() const { return _oledDisplayTask.get(); }
    
    // Cleanup
    void cleanupAll();
    void cleanupSystemMonitorTask();
    void cleanupOLEDDisplayTask();
    // Check if tasks are running
    bool isSystemMonitorTaskRunning() const;
    bool isOLEDDisplayTaskRunning() const;
private:
    TaskManager() = default;
    ~TaskManager() { cleanupAll(); }
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;
    
    // Task pointers
    std::unique_ptr<SystemMonitorTask> _systemMonitorTask;
    std::unique_ptr<OLEDDisplayTask> _oledDisplayTask;
};

