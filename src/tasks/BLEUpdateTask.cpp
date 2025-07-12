#include "BLEUpdateTask.h"
#include "BLEManager.h"

BLEUpdateTask::BLEUpdateTask(BLEManager& manager) : bleManager(manager) {
}

void BLEUpdateTask::update() {
    bleManager.update();
} 