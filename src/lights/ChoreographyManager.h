#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <array>
#include "../controllers/BrightnessController.h"
#include "../GlobalState.h"
#include "freertos/LogManager.h"
#include "RingPlayer.h"
#include "PulsePlayer.h"

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
    // Beat pattern structure - generic to support any action type
    struct BeatPattern {
        String id;
        float bps;  // Beats per second (e.g., 1.0 = 1 beat/sec)
        String action;  // Action type: "brightness_pulse", "fire_ring", "fire_pulse", etc.
        String paramsJson;  // JSON string of action parameters (parsed when needed)
        unsigned long startTime;  // Relative to choreography start (ms)
        unsigned long endTime;    // When this beat pattern ends (ms, relative to choreography start)
        unsigned long lastBeatTime;  // When we last fired this beat
        bool active;
        
        unsigned long getIntervalMs() const {
            return (unsigned long)(1000.0f / bps);
        }
    };
    
    // Timeline event structure - one-off actions at specific timestamps
    struct TimelineEvent {
        unsigned long time;  // When to fire (ms from choreography start)
        String action;       // Action type: "change_effect", "set_brightness", "fire_ring", "fire_pulse", etc.
        String paramsJson;  // JSON string of action parameters (parsed when needed)
        bool executed;       // Has this event fired? (ensures it fires once and only once)
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
    std::vector<TimelineEvent> timelineEvents;
    SavedState savedState;
    
    unsigned long choreographyStartTime;
    unsigned long choreographyDuration;
    bool active;
    
    // Built-in count-in duration (3 seconds)
    static constexpr unsigned long COUNT_IN_DURATION = 3100;
    unsigned long lastCountInPulseTime;  // Track last count-in pulse to avoid firing multiple times
    
    EffectManager* effectManager;
    
    // Ring player pool for fire_ring actions
    static constexpr int RING_PLAYER_POOL_SIZE = 15;
    std::array<RingPlayer, RING_PLAYER_POOL_SIZE> ringPlayerPool;
    Light* outputBuffer;
    int gridRows;
    int gridCols;
    bool ringPlayersInitialized;

    // Pulse player pool for fire_pulse actions (round-robin)
    static constexpr int PULSE_PLAYER_POOL_SIZE = 15;
    std::array<PulsePlayer, PULSE_PLAYER_POOL_SIZE> pulsePlayerPool;
    int numLEDs;
    bool pulsePlayersInitialized;
    int nextPulsePlayerIdx;
    
    // Methods
    void saveCurrentState();
    void restorePreviousState();
    void updateBeatPatterns(unsigned long timelineElapsed);
    void updateTimelineEvents(unsigned long timelineElapsed);
    void executeBeatAction(BeatPattern& beat);
    void executeEventAction(TimelineEvent& event);
    void executeBrightnessPulse(const JsonObject& params);
    void executeFireRing(const JsonObject& params);
    void executeChangeEffect(const JsonObject& params);
    void executeSetBrightness(const JsonObject& params);
    void executeUpdateEffectParams(const JsonObject& params);
    void executeFirePulse(const JsonObject& params);
    void initializeRingPlayers(Light* buffer, int rows, int cols);
    void initializePulsePlayers(Light* buffer, int numLeds);
    RingPlayer* findAvailableRingPlayer();
    
    // Helper function to parse time strings (e.g., "0:45.500" or "45.500") to milliseconds
    static unsigned long parseTimeString(const JsonVariant& timeValue);
};
