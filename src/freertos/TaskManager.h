#pragma once

#include "PlatformConfig.h"
#include "SRTask.h"

// Forward declarations
class SystemMonitorTask;

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
    
    // Accessors - return nullptr if task not created
    SystemMonitorTask* getSystemMonitorTask() const { return _systemMonitorTask; }
    
    // Cleanup
    void cleanupAll();
    void cleanupSystemMonitorTask();
    
    // Check if tasks are running
    bool isSystemMonitorTaskRunning() const;

private:
    TaskManager() = default;
    ~TaskManager() { cleanupAll(); }
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;
    
    // Task pointers
    SystemMonitorTask* _systemMonitorTask = nullptr;
};

