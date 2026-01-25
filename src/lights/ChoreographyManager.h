#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "../controllers/BrightnessController.h"
#include "../GlobalState.h"
#include "freertos/LogManager.h"

// Forward declaration
class EffectManager;

class ChoreographyManager {
public:
    ChoreographyManager();
    ~ChoreographyManager();
    
    // Lifecycle
    void startChoreography(const JsonObject& command, EffectManager* effectManager);
    void update(float dt);
    void render(Light* outputBuffer, int numLEDs, int gridRows = 32, int gridCols = 32);
    void stop();
    bool isActive() const { return active; }
    
private:
    // Beat pattern structure for brightness pulsing
    struct BeatPattern {
        String id;
        float bps;  // Beats per second (e.g., 1.0 = 1 beat/sec)
        int baseBrightness;
        int peakBrightness;
        unsigned long pulseDuration;  // Duration of each pulse in ms (e.g., 250ms)
        unsigned long startTime;  // Relative to choreography start (ms)
        unsigned long duration;   // How long this beat pattern runs (ms)
        unsigned long lastBeatTime;  // When we last fired this beat
        bool active;
        
        unsigned long getIntervalMs() const {
            return (unsigned long)(1000.0f / bps);
        }
    };
    
    // Saved state for restoration
    struct SavedState {
        String effectType;
        String effectParams;
        int brightness;
        bool valid;
    };
    
    // Choreography state
    std::vector<BeatPattern> beatPatterns;
    SavedState savedState;
    
    unsigned long choreographyStartTime;
    unsigned long choreographyDuration;
    bool active;
    
    EffectManager* effectManager;
    
    // Methods
    void saveCurrentState();
    void restorePreviousState();
    void updateBeatPatterns();
    void executeBrightnessPulse(BeatPattern& beat);
};
