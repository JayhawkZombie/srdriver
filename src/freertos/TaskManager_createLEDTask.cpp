#include "TaskManager.h"
#include "LogManager.h"
#include "LEDUpdateTask.h"

bool TaskManager::createLEDTask(uint32_t updateIntervalMs)
{
    // Note: We allow LED task creation even without SUPPORTS_LEDS
    // The task will just sleep if there's no LED manager or hardware
    if (_ledTask != nullptr)
    {
        LOG_WARN_COMPONENT("TaskManager", "LED task already created");
        return _ledTask->isRunning();
    }

    _ledTask = new LEDUpdateTask(updateIntervalMs);
    if (_ledTask->start())
    {
        LOG_INFO_COMPONENT("TaskManager", "LED task created and started");
        return true;
    }
    else
    {
        LOG_ERROR_COMPONENT("TaskManager", "Failed to start LED task");
        delete _ledTask;
        _ledTask = nullptr;
        return false;
    }
}

void TaskManager::cleanupLEDTask()
{
    if (_ledTask)
    {
        _ledTask->stop();
        delete _ledTask;
        _ledTask = nullptr;
        LOG_INFO_COMPONENT("TaskManager", "LED task cleaned up");
    }
}

bool TaskManager::isLEDTaskRunning() const
{
    return _ledTask != nullptr && _ledTask->isRunning();
}
