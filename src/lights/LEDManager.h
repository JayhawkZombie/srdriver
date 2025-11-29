#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <memory>
#include <vector>
#include "Light.h"
#include "freertos/SRSmartQueue.h"

// Test structure for smart queue
struct TestCommand {
    std::shared_ptr<DynamicJsonDocument> doc;
    uint32_t timestamp;
};

// Forward declarations for sub-managers
class EffectManager;
class SequenceManager;
class ChoreographyManager;

enum class LEDManagerState {
    IDLE,
    EFFECT_PLAYING,
    SEQUENCE_PLAYING,
    CHOREOGRAPHY_PLAYING,
    EMERGENCY
};

class LEDManager {
public:
    LEDManager();
    ~LEDManager();
    
    void update(float dtSeconds);
    void render(Light* output, int numLEDs);
    
    // State machine interface
    void transitionTo(LEDManagerState newState);
    LEDManagerState getCurrentState() const { 
        return stateStack.empty() ? LEDManagerState::IDLE : stateStack.back(); 
    }
    
    // JSON command interface
    void handleCommand(const JsonObject& command);
    
    // TEST: Smart queue test methods
    bool testQueueCommand(std::shared_ptr<DynamicJsonDocument> doc);  // Test sending to queue
    void testProcessQueue();  // Test receiving from queue (call from update/render)
    
    // Brightness control
    void setBrightness(int brightness);
    int getBrightness() const;
    
    // State stack management
    void pushState(LEDManagerState newState);
    void popState();
    void popState(LEDManagerState expectedState);
    void replaceState(LEDManagerState newState);
    int getStateStackDepth() const { return stateStack.size(); }
    
private:
    // State stack - top of stack is current state
    std::vector<LEDManagerState> stateStack;
    
    // Brightness tracking
    int currentBrightness = 128;
    
    // TEST: Smart queue for testing
    SRSmartQueue<TestCommand> testQueue;
    
    // Sub-managers
    std::unique_ptr<EffectManager> effectManager;
    // std::unique_ptr<SequenceManager> sequenceManager;
    // std::unique_ptr<ChoreographyManager> choreographyManager;
    
    // State machine logic
    void onStateEnter(LEDManagerState state);
    void onStateExit(LEDManagerState state);
    void onStateUpdate(LEDManagerState state, float dtSeconds);
    
    // Command routing
    void handleEffectCommand(const JsonObject& command);
    void handleSequenceCommand(const JsonObject& command);
    void handleChoreographyCommand(const JsonObject& command);
    void handleEmergencyCommand(const JsonObject& command);
    
    // Simple white LED effect
    void renderWhiteLEDs(Light* output, int numLEDs);
};
