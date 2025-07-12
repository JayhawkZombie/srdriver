#pragma once

// Forward declaration
class BLEManager;

class BLEUpdateTask {
public:
    BLEUpdateTask(BLEManager& manager);
    void update();
private:
    BLEManager& bleManager;
}; 