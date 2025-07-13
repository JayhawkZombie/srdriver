#include "LEDUpdateTask.h"
#include "PatternManager.h"

LEDUpdateTask::LEDUpdateTask() {
    // Constructor - nothing to initialize
}

void LEDUpdateTask::update() {
    Pattern_Loop();
} 