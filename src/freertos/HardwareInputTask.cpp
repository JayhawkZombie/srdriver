#include "HardwareInputTask.h"

// Implementation of build methods for HardwareInputTaskBuilder

HardwareInputTask* HardwareInputTaskBuilder::build() {
    if (!isValid()) {
        Serial.println("[HardwareInputTaskBuilder] No valid configuration found");
        return nullptr;
    }
    return new HardwareInputTask(configs);
}

std::unique_ptr<HardwareInputTask> HardwareInputTaskBuilder::buildUnique() {
    if (!isValid()) {
        Serial.println("[HardwareInputTaskBuilder] No valid configuration found");
        return nullptr;
    }
    return std::unique_ptr<HardwareInputTask>(new HardwareInputTask(configs));
} 