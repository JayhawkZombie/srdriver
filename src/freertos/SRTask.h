#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "LogManager.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

/**
 * SRTask - Base class for FreeRTOS tasks
 * 
 * Provides a clean C++ interface for creating and managing FreeRTOS tasks.
 * Each derived class should implement the run() method with the task's main logic.
 */
class SRTask {
public:
    SRTask(const char* name, uint32_t stackSize = 4096, 
           UBaseType_t priority = tskIDLE_PRIORITY + 1,
           BaseType_t core = tskNO_AFFINITY)
        : _name(name), _stackSize(stackSize), _priority(priority), 
          _core(core), _handle(nullptr), _running(false) {}

    virtual ~SRTask() {
        stop();
    }

    /**
     * Start the task
     * @return true if task was created successfully
     */
    bool start() {
        if (_running) return true;
        
        BaseType_t result;
        if (_core == tskNO_AFFINITY) {
            result = xTaskCreate(_taskEntry, _name, _stackSize, this, _priority, &_handle);
        } else {
            result = xTaskCreatePinnedToCore(_taskEntry, _name, _stackSize, this, 
                                           _priority, &_handle, _core);
        }
        
        if (result == pdPASS) {
            _running = true;
            LOG_INFOF_COMPONENT("SRTask", "Started task '%s' on core %d", _name, _core);
            return true;
        } else {
            LOG_ERRORF_COMPONENT("SRTask", "Failed to start task '%s'", _name);
            return false;
        }
    }

    /**
     * Stop the task
     */
    void stop() {
        if (_handle && _running) {
            vTaskDelete(_handle);
            _handle = nullptr;
            _running = false;
            LOG_INFOF_COMPONENT("SRTask", "Stopped task '%s'", _name);
        }
    }

    /**
     * Check if task is running
     */
    bool isRunning() const { return _running; }

    /**
     * Get task name
     */
    const char* getName() const { return _name; }

    /**
     * Get task handle
     */
    TaskHandle_t getHandle() const { return _handle; }

    /**
     * Sleep for specified milliseconds
     */
    static void sleep(uint32_t ms) {
        vTaskDelay(ms / portTICK_PERIOD_MS);
    }

    /**
     * Sleep until specified time (for periodic tasks)
     */
    static void sleepUntil(TickType_t* lastWakeTime, uint32_t periodMs) {
        vTaskDelayUntil(lastWakeTime, periodMs / portTICK_PERIOD_MS);
    }

    /**
     * Yield to other tasks
     */
    static void yield() {
        taskYIELD();
    }

protected:
    /**
     * Main task function - override this in derived classes
     */
    virtual void run() = 0;

    /**
     * Task entry point (static wrapper)
     */
    static void _taskEntry(void* parameter) {
        SRTask* task = static_cast<SRTask*>(parameter);
        task->run();
    }

    const char* _name;
    uint32_t _stackSize;
    UBaseType_t _priority;
    BaseType_t _core;
    TaskHandle_t _handle;
    bool _running;
}; 