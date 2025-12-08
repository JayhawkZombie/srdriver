#include "LEDManager.h"
#include "freertos/LogManager.h"
#include "../controllers/BrightnessController.h"
#include <FastLED.h>
#include "effects/EffectManager.h"
#include "effects/EffectFactory.h"
#include "../GlobalState.h"
#include "../PatternManager.h"
#include "../Globals.h"

LEDManager::LEDManager() 
    : commandQueue(10, "LEDCommandQueue")  // TEST: Initialize test queue
{
    LOG_DEBUGF_COMPONENT("LEDManager", "Initializing");
    
    // Initialize sub-managers
    effectManager = std::unique_ptr<EffectManager>(new EffectManager());
    /*
        for (auto &panel : lightPanels) {
        panel.init_Src(buffer, 32, 32);
        panel.type = 2;
    }
    lightPanels[0].set_SrcArea(16, 16, 0, 0);
    lightPanels[0].pTgt0 = output;
    lightPanels[0].rotIdx = 1;
    lightPanels[1].set_SrcArea(16, 16, 0, 16);
    lightPanels[1].pTgt0 = output + 256;
    lightPanels[1].rotIdx = -1;
    lightPanels[2].set_SrcArea(16, 16, 16, 0);
    lightPanels[2].pTgt0 = output + 512;
    lightPanels[2].rotIdx = 1;
    lightPanels[3].set_SrcArea(16, 16, 16, 16);
    lightPanels[3].pTgt0 = output + 768;
    lightPanels[3].rotIdx = 1;*/
    std::vector<PanelConfig> pc = {
        {16, 16, 0, 0, 2, 1, false},
        {16, 16, 0, 16, 2, -1, false},
        {16, 16, 16, 0, 2, 1, false},
        {16, 16, 16, 16, 2, 1, false}
    };
    initPanels(pc);
    // Use blendLightArr for the buffer
    // sequenceManager = std::make_unique<SequenceManager>();
    // choreographyManager = std::make_unique<ChoreographyManager>();
    
    // Start with IDLE state on the stack
    pushState(LEDManagerState::IDLE);
}

LEDManager::~LEDManager() {
    LOG_DEBUGF_COMPONENT("LEDManager", "Destroying");
}

void LEDManager::initPanels(const std::vector<PanelConfig>& panelConfigs) {
    _panelConfigs = panelConfigs;
    _lightPanels.resize(panelConfigs.size());
    for (size_t i = 0; i < panelConfigs.size(); i++) {
        _lightPanels[i].init_Src(BlendLightArr, 32, 32);
        _lightPanels[i].set_SrcArea(panelConfigs[i].rows, panelConfigs[i].cols, panelConfigs[i].row0, panelConfigs[i].col0);
        _lightPanels[i].pTgt0 = LightArr + i * panelConfigs[i].rows * panelConfigs[i].cols;
        _lightPanels[i].rotIdx = panelConfigs[i].rotIdx;
        _lightPanels[i].swapTgtRCs = panelConfigs[i].swapTgtRCs;
        _lightPanels[i].type = panelConfigs[i].type;
    }
    // for (auto &panel : _lightPanels)
    // {
    //     panel.init_Src(BlendLightArr, 32, 32);
    //     panel.type = 2;
    // }
    // _lightPanels[0].set_SrcArea(16, 16, 0, 0);
    // _lightPanels[0].pTgt0 = LightArr;
    // _lightPanels[0].rotIdx = 1;
    // _lightPanels[1].set_SrcArea(16, 16, 0, 16);
    // _lightPanels[1].pTgt0 = LightArr + 256;
    // _lightPanels[1].rotIdx = -1;
    // _lightPanels[2].set_SrcArea(16, 16, 16, 0);
    // _lightPanels[2].pTgt0 = LightArr + 512;
    // _lightPanels[2].rotIdx = 1;
    // _lightPanels[3].set_SrcArea(16, 16, 16, 16);
    // _lightPanels[3].pTgt0 = LightArr + 768;
    // _lightPanels[3].rotIdx = 1;
    useLightPanels = true;
}

void LEDManager:: update(float dtSeconds, Light* output, int numLEDs) {
    // Sync brightness from BrightnessController
    int brightnessFromController = getBrightness();
    if (brightnessFromController != currentBrightness) {
        currentBrightness = brightnessFromController;
    }
    
    // Always call FastLED.setBrightness() to ensure we never lose track
    FastLED.setBrightness(currentBrightness);
    
    // Update current state
    onStateUpdate(getCurrentState(), dtSeconds);

    // Update sub-managers
    if (effectManager) {
        effectManager->update(dtSeconds);
        // effectManager->render(output, numLEDs);
    }
    render(output, numLEDs);
    // if (sequenceManager) sequenceManager->update(dtSeconds);
    // if (choreographyManager) choreographyManager->update(dtSeconds);
}

void LEDManager::render(Light* output, int numLEDs) {
    // Clear output
    for (int i = 0; i < numLEDs; i++) {
        output[i] = Light(0, 0, 0);
    }
    
    // Render based on current state (top of stack)
    LEDManagerState currentState = getCurrentState();
    switch (currentState) {
        case LEDManagerState::IDLE:
            // Nothing to render
            break;
            
        case LEDManagerState::EFFECT_PLAYING:
            // Render effects through EffectManager
            if (effectManager) {
                effectManager->render(BlendLightArr, numLEDs);
            } else {
                // Fallback to simple white LEDs if no EffectManager
                renderWhiteLEDs(BlendLightArr, numLEDs);
            }
            break;
            
        case LEDManagerState::SEQUENCE_PLAYING:
            // TODO: Render sequences when SequenceManager is built
            // if (sequenceManager) sequenceManager->render(output, numLEDs);
            break;
            
        case LEDManagerState::CHOREOGRAPHY_PLAYING:
            // TODO: Render choreography when ChoreographyManager is built
            // if (choreographyManager) choreographyManager->render(output, numLEDs);
            break;
            
        case LEDManagerState::EMERGENCY:
            // Render emergency pattern
            for (int i = 0; i < numLEDs; i++) {
                output[i] = Light(255, 0, 0);  // Red alert
            }
            break;
    }

    _lightPanels[0].pTgt0 = output;
    _lightPanels[1].pTgt0 = output + 256;
    _lightPanels[2].pTgt0 = output + 512;
    _lightPanels[3].pTgt0 = output + 768;

    if (useLightPanels)
    {
        for (auto &panel : _lightPanels)
        {
            panel.update();
        }
    }
    else
    {
        // Render directly to LightArr
        for (int i = 0; i < numLEDs; i++)
        {
            output[i] = BlendLightArr[i];
        }
    }
}

void LEDManager::transitionTo(LEDManagerState newState) {
    LEDManagerState currentState = getCurrentState();
    if (newState == currentState) return;
    
    LOG_DEBUGF_COMPONENT("LEDManager", "Transitioning from %d to %d", (int)currentState, (int)newState);
    
    // Exit current state
    onStateExit(currentState);
    
    // Replace current state (top of stack)
    if (!stateStack.empty()) {
        stateStack.back() = newState;
    } else {
        stateStack.push_back(newState);
    }
    
    // Enter new state
    onStateEnter(newState);
}

void LEDManager::onStateEnter(LEDManagerState state) {
    switch (state) {
        case LEDManagerState::IDLE:
            LOG_DEBUGF_COMPONENT("LEDManager", "Entering IDLE state");
            break;
            
        case LEDManagerState::EFFECT_PLAYING:
            LOG_DEBUGF_COMPONENT("LEDManager", "Entering EFFECT_PLAYING state");
            break;
            
        case LEDManagerState::SEQUENCE_PLAYING:
            LOG_DEBUGF_COMPONENT("LEDManager", "Entering SEQUENCE_PLAYING state");
            break;
            
        case LEDManagerState::CHOREOGRAPHY_PLAYING:
            LOG_DEBUGF_COMPONENT("LEDManager", "Entering CHOREOGRAPHY_PLAYING state");
            break;
            
        case LEDManagerState::EMERGENCY:
            LOG_DEBUGF_COMPONENT("LEDManager", "Entering EMERGENCY state");
            break;
    }
}

void LEDManager::onStateExit(LEDManagerState state) {
    switch (state) {
        case LEDManagerState::IDLE:
            LOG_DEBUGF_COMPONENT("LEDManager", "Exiting IDLE state");
            break;
            
        case LEDManagerState::EFFECT_PLAYING:
            LOG_DEBUGF_COMPONENT("LEDManager", "Exiting EFFECT_PLAYING state");
            break;
            
        case LEDManagerState::SEQUENCE_PLAYING:
            LOG_DEBUGF_COMPONENT("LEDManager", "Exiting SEQUENCE_PLAYING state");
            break;
            
        case LEDManagerState::CHOREOGRAPHY_PLAYING:
            LOG_DEBUGF_COMPONENT("LEDManager", "Exiting CHOREOGRAPHY_PLAYING state");
            break;
            
        case LEDManagerState::EMERGENCY:
            LOG_DEBUGF_COMPONENT("LEDManager", "Exiting EMERGENCY state");
            break;
    }
}

void LEDManager::onStateUpdate(LEDManagerState state, float dtSeconds) {
    switch (state) {
        case LEDManagerState::IDLE:
            // Nothing to do in IDLE
            break;
            
        case LEDManagerState::EFFECT_PLAYING:
            // TODO: Effects are updated by EffectManager
            break;
            
        case LEDManagerState::SEQUENCE_PLAYING:
            // TODO: Sequences are updated by SequenceManager
            break;
            
        case LEDManagerState::CHOREOGRAPHY_PLAYING:
            // TODO: Choreography is updated by ChoreographyManager
            break;
            
        case LEDManagerState::EMERGENCY:
            // Emergency state - just keep rendering red
            break;
    }
}

void LEDManager::handleCommand(const JsonObject& command) {
    String commandType = "";
    if (command.containsKey("type")) {
        commandType = String(command["type"].as<const char*>());
    } else if (command.containsKey("t")) {
        commandType = String(command["t"].as<const char*>());
    }
    // LOG_DEBUGF_COMPONENT("LEDManager", "Handling command type: %s", commandType.c_str());
    if (commandType == "effect") {
        // const auto startTime = micros();
        handleEffectCommand(command);
        // const auto endTime = micros();
        // const auto duration = endTime - startTime;
        // LOG_DEBUGF_COMPONENT("LEDManager", "Took %lu us to handle command type %s", duration, commandType.c_str());
    }
    else if (commandType == "sequence") {
        handleSequenceCommand(command);
    }
    else if (commandType == "choreography") {
        handleChoreographyCommand(command);
    }
    else if (commandType == "emergency") {
        handleEmergencyCommand(command);
    }
    else {
        LOG_ERRORF_COMPONENT("LEDManager", "Unknown command type: %s", commandType.c_str());
    }
}

void LEDManager::handleEffectCommand(const JsonObject& command) {
    LOG_DEBUGF_COMPONENT("LEDManager", "Handling effect command");
    const auto startTime = micros();
    transitionTo(LEDManagerState::EFFECT_PLAYING);
    
    // Create effect using EffectFactory
    if (effectManager) {
        // LOG_DEBUGF_COMPONENT("LEDManager", "Cleared all existing effects");
        
        JsonObject effectCommand;
        if (command.containsKey("effect")) {
            effectCommand = command["effect"].as<JsonObject>();
        } else if (command.containsKey("e")) {
            effectCommand = command["e"].as<JsonObject>();
        }
        
        // Save current effect to device state
        if (effectCommand.containsKey("type")) {
            deviceState.currentEffectType = effectCommand["type"].as<String>();
        } else if (effectCommand.containsKey("t")) {
            deviceState.currentEffectType = effectCommand["t"].as<String>();
        }
        
        const auto effectCreationStartTime = micros();
        auto effect = EffectFactory::createEffect(effectCommand);
        const auto effectCreationEndTime = micros();
        const auto effectCreationDuration = effectCreationEndTime - effectCreationStartTime;
        LOG_DEBUGF_COMPONENT("LEDManager", "Took %lu us to create effect", effectCreationDuration);
        if (effect) {
            // const auto removeAllEffectsStartTime = micros();
            // Clear all existing effects before adding new one
            effectManager->removeAllEffects();
            // const auto removeAllEffectsEndTime = micros();
            // const auto removeAllEffectsDuration = removeAllEffectsEndTime - removeAllEffectsStartTime;
            // LOG_DEBUGF_COMPONENT("LEDManager", "Took %lu us to remove all effects", removeAllEffectsDuration);
            // const auto effectAddStartTime = micros();
            effectManager->addEffect(std::move(effect));
            // const auto effectAddEndTime = micros();
            // const auto effectAddDuration = effectAddEndTime - effectAddStartTime;
            // LOG_DEBUGF_COMPONENT("LEDManager", "Took %lu us to add effect to EffectManager", effectAddDuration);
            
            // Save preferences after successful effect creation
            // Save effect parameters as JSON string
            String effectParams = "";
            if (effectCommand.containsKey("parameters"))
            {
                serializeJson(effectCommand["parameters"], effectParams);
            }
            else if (effectCommand.containsKey("p"))
            {
                serializeJson(effectCommand["p"], effectParams);
            }
            deviceState.currentEffectParams = effectParams;

            // LOG_DEBUGF_COMPONENT("LEDManager", "Saved effect - Type: %s, Params: %s",
            //     deviceState.currentEffectType.c_str(), deviceState.currentEffectParams.c_str());
            SaveUserPreferences(deviceState);
            // LOG_DEBUGF_COMPONENT("LEDManager", "Saved user preferences with current effect");
        } else {
            LOG_ERRORF_COMPONENT("LEDManager", "Failed to create effect");
        }
    } else {
        LOG_ERRORF_COMPONENT("LEDManager", "EffectManager not available");
    }
    // const auto endTime = micros();
    // const auto duration = endTime - startTime;
    // LOG_DEBUGF_COMPONENT("LEDManager", "Took %lu us to handle effect command", duration);
}

void LEDManager::handleSequenceCommand(const JsonObject& command) {
    LOG_DEBUGF_COMPONENT("LEDManager", "Handling sequence command");
    transitionTo(LEDManagerState::SEQUENCE_PLAYING);
    // TODO: Play sequence when SequenceManager is built
    // if (sequenceManager) {
    //     sequenceManager->playSequence(command["timeline"]);
    // }
}

void LEDManager::handleChoreographyCommand(const JsonObject& command) {
    LOG_DEBUGF_COMPONENT("LEDManager", "Handling choreography command");
    transitionTo(LEDManagerState::CHOREOGRAPHY_PLAYING);
    // TODO: Play choreography when ChoreographyManager is built
    // if (choreographyManager) {
    //     choreographyManager->playChoreography(command);
    // }
}

void LEDManager::handleEmergencyCommand(const JsonObject& command) {
    LOG_DEBUGF_COMPONENT("LEDManager", "Handling emergency command");
    pushState(LEDManagerState::EMERGENCY);
}

void LEDManager::setBrightness(int brightness) {
    // Clamp brightness to valid range
    brightness = constrain(brightness, 0, 255);
    
    if (brightness != currentBrightness) {
        currentBrightness = brightness;
        LOG_DEBUGF_COMPONENT("LEDManager", "Brightness set to %d", brightness);
    }
}

int LEDManager::getBrightness() const {
    // Fetch brightness from BrightnessController (single source of truth)
    BrightnessController* brightnessController = BrightnessController::getInstance();
    if (brightnessController) {
        return brightnessController->getBrightness();
    }
    
    // Fallback to internal value if BrightnessController not available
    return currentBrightness;
}

void LEDManager::pushState(LEDManagerState newState) {
    LOG_DEBUGF_COMPONENT("LEDManager", "Pushing state: %d (stack depth: %d)", (int)newState, stateStack.size());
    
    // Exit current state if stack not empty
    if (!stateStack.empty()) {
        onStateExit(getCurrentState());
    }
    
    // Push new state onto stack
    stateStack.push_back(newState);
    
    // Enter new state
    onStateEnter(newState);
}

void LEDManager::popState() {
    if (stateStack.empty()) {
        LOG_WARNF_COMPONENT("LEDManager", "Cannot pop state - stack is empty");
        return;
    }
    
    LEDManagerState currentState = getCurrentState();
    LOG_DEBUGF_COMPONENT("LEDManager", "Popping state: %d (stack depth: %d)", (int)currentState, stateStack.size());
    
    // Exit current state
    onStateExit(currentState);
    
    // Pop current state
    stateStack.pop_back();
    
    // Enter previous state (if any)
    if (!stateStack.empty()) {
        onStateEnter(getCurrentState());
    }
}

void LEDManager::popState(LEDManagerState expectedState) {
    if (stateStack.empty()) {
        LOG_WARNF_COMPONENT("LEDManager", "Cannot pop state - stack is empty");
        return;
    }
    
    LEDManagerState currentState = getCurrentState();
    if (currentState != expectedState) {
        LOG_WARNF_COMPONENT("LEDManager", "Expected to pop state %d but current state is %d - ignoring", (int)expectedState, (int)currentState);
        return;
    }
    
    LOG_DEBUGF_COMPONENT("LEDManager", "Popping expected state: %d (stack depth: %d)", (int)currentState, stateStack.size());
    
    // Exit current state
    onStateExit(currentState);
    
    // Pop current state
    stateStack.pop_back();
    
    // Enter previous state (if any)
    if (!stateStack.empty()) {
        onStateEnter(getCurrentState());
    }
}

void LEDManager::replaceState(LEDManagerState newState) {
    LOG_DEBUGF_COMPONENT("LEDManager", "Replacing state with: %d", (int)newState);
    
    // Exit current state if stack not empty
    if (!stateStack.empty()) {
        onStateExit(getCurrentState());
    }
    
    // Replace or set new state
    if (!stateStack.empty()) {
        stateStack.back() = newState;
    } else {
        stateStack.push_back(newState);
    }
    
    // Enter new state
    onStateEnter(newState);
}

void LEDManager::renderWhiteLEDs(Light* output, int numLEDs) {
    // Render pure white LEDs (brightness is controlled by FastLED.setBrightness)
    for (int i = 0; i < numLEDs; i++) {
        output[i] = Light(255, 255, 255);
    }
}

// TEST: Smart queue test methods
bool LEDManager::safeQueueCommand(std::shared_ptr<DynamicJsonDocument> doc) {
    if (!doc) {
        LOG_ERROR_COMPONENT("LEDManager", "TEST: Cannot queue null document");
        return false;
    }
    
    TestCommand cmd;
    cmd.doc = doc;  // shared_ptr copy
    cmd.timestamp = millis();
    
    bool queued = commandQueue.send(std::move(cmd));
    return queued;
}

void LEDManager::safeProcessQueue() {
    TestCommand cmd;
    uint32_t queueCount = 0;
    while (commandQueue.receive(cmd, 0)) {  // Non-blocking, process all
        const auto receiveStartTime = micros();
        queueCount++;
        
        if (cmd.doc) {
            JsonObject root = cmd.doc->as<JsonObject>();
            String type = "";
            if (root.containsKey("type")) {
                type = root["type"].as<String>();
            } else if (root.containsKey("t")) {
                type = root["t"].as<String>();
            }
            
            const auto startTime = micros();
            handleCommand(root);
            const auto endTime = micros();
            const auto duration = endTime - startTime;
            // LOG_DEBUGF_COMPONENT("LEDManager", "Gap between receive and handle command: %lu us", startTime - receiveStartTime);
            
            // LOG_DEBUGF_COMPONENT("LEDManager", "Processed %s command in %lu us (queued for %lu ms)", 
            //           type.c_str(), duration, millis() - cmd.timestamp);
        }
        // shared_ptr automatically cleans up when cmd goes out of scope
    }
    
    if (queueCount > 0) {
        // LOG_DEBUGF_COMPONENT("LEDManager", "Processed %d command(s) from queue", queueCount);
    }
}
