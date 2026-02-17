#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <memory>
#include <vector>
#include "Light.h"
#include "LightPanel.h"
#include "freertos/SRSmartQueue.h"
#include "hal/network/ICommandHandler.h"
#include "../Globals.h"

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

class LEDManager : public ICommandHandler {
public:
    LEDManager();
    ~LEDManager();
    
    void update(float dtSeconds, Light* output, int numLEDs);
    void render(Light* output, int numLEDs);
    
    // State machine interface
    void transitionTo(LEDManagerState newState);
    LEDManagerState getCurrentState() const { 
        return stateStack.empty() ? LEDManagerState::IDLE : stateStack.back(); 
    }
    
    // ICommandHandler interface
    bool handleCommand(const JsonObject& command) override;
    bool supportsQueuing() const override { return true; }
    bool handleQueuedCommand(std::shared_ptr<DynamicJsonDocument> doc) override;
    int getBrightness() const override;
    String getStatus() const override;
    
    // TEST: Smart queue test methods
    bool safeQueueCommand(std::shared_ptr<DynamicJsonDocument> doc);  // thread-safe sending to queue
    void safeProcessQueue();  // thread-safe receiving from queue (call from update/render)
    
    // Brightness control
    void setBrightness(int brightness);
    
    // LED count configuration
    void setNumConfiguredLEDs(int numLEDs);
    
    // State stack management
    void pushState(LEDManagerState newState);
    void popState();
    void popState(LEDManagerState expectedState);
    void replaceState(LEDManagerState newState);
    int getStateStackDepth() const { return stateStack.size(); }
    void initPanels(const std::vector<PanelConfig>& panelConfigs);
    
private:

    // If using panels, we use blendLightArr for the buffer
    // otherwise we render directly to LightArr
    bool useLightPanels = false;
    std::vector<LightPanel> _lightPanels;
    std::vector<PanelConfig> _panelConfigs;

    // State stack - top of stack is current state
    std::vector<LEDManagerState> stateStack;
    
    // Brightness tracking
    int currentBrightness = 128;
    
    // LED count configuration (from SD card config)
    int _numConfiguredLEDs = NUM_LEDS;
    
    // thread-safe queue for testing
    SRSmartQueue<TestCommand> commandQueue;
    
    // Sub-managers
    std::unique_ptr<EffectManager> effectManager;
    // std::unique_ptr<SequenceManager> sequenceManager;
    std::unique_ptr<ChoreographyManager> choreographyManager;
    
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
