#include "LEDManager.h"
#include "freertos/LogManager.h"
#include "../controllers/BrightnessController.h"
#include <FastLED.h>

LEDManager::LEDManager() {
    LOG_DEBUG("LEDManager: Initializing");
    
    // TODO: Initialize sub-managers when we build them
    // effectManager = std::make_unique<EffectManager>();
    // sequenceManager = std::make_unique<SequenceManager>();
    // choreographyManager = std::make_unique<ChoreographyManager>();
    
    // Start with IDLE state on the stack
    pushState(LEDManagerState::IDLE);
}

LEDManager::~LEDManager() {
    LOG_DEBUG("LEDManager: Destroying");
}

void LEDManager::update(float dtSeconds) {
    // Sync brightness from BrightnessController
    int brightnessFromController = getBrightness();
    if (brightnessFromController != currentBrightness) {
        currentBrightness = brightnessFromController;
    }
    
    // Always call FastLED.setBrightness() to ensure we never lose track
    FastLED.setBrightness(currentBrightness);
    
    // Update current state
    onStateUpdate(getCurrentState(), dtSeconds);
    
    // TODO: Update sub-managers when we build them
    // if (effectManager) effectManager->update(dtSeconds);
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
            // For now, render simple white LEDs with brightness control
            renderWhiteLEDs(output, numLEDs);
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
}

void LEDManager::transitionTo(LEDManagerState newState) {
    LEDManagerState currentState = getCurrentState();
    if (newState == currentState) return;
    
    LOG_DEBUG("LEDManager: Transitioning from " + String((int)currentState) + " to " + String((int)newState));
    
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
            LOG_DEBUG("LEDManager: Entering IDLE state");
            break;
            
        case LEDManagerState::EFFECT_PLAYING:
            LOG_DEBUG("LEDManager: Entering EFFECT_PLAYING state");
            break;
            
        case LEDManagerState::SEQUENCE_PLAYING:
            LOG_DEBUG("LEDManager: Entering SEQUENCE_PLAYING state");
            break;
            
        case LEDManagerState::CHOREOGRAPHY_PLAYING:
            LOG_DEBUG("LEDManager: Entering CHOREOGRAPHY_PLAYING state");
            break;
            
        case LEDManagerState::EMERGENCY:
            LOG_DEBUG("LEDManager: Entering EMERGENCY state");
            break;
    }
}

void LEDManager::onStateExit(LEDManagerState state) {
    switch (state) {
        case LEDManagerState::IDLE:
            LOG_DEBUG("LEDManager: Exiting IDLE state");
            break;
            
        case LEDManagerState::EFFECT_PLAYING:
            LOG_DEBUG("LEDManager: Exiting EFFECT_PLAYING state");
            break;
            
        case LEDManagerState::SEQUENCE_PLAYING:
            LOG_DEBUG("LEDManager: Exiting SEQUENCE_PLAYING state");
            break;
            
        case LEDManagerState::CHOREOGRAPHY_PLAYING:
            LOG_DEBUG("LEDManager: Exiting CHOREOGRAPHY_PLAYING state");
            break;
            
        case LEDManagerState::EMERGENCY:
            LOG_DEBUG("LEDManager: Exiting EMERGENCY state");
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
    String commandType = command["type"];
    
    if (commandType == "effect") {
        handleEffectCommand(command);
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
        LOG_ERROR("LEDManager: Unknown command type: " + commandType);
    }
}

void LEDManager::handleEffectCommand(const JsonObject& command) {
    LOG_DEBUG("LEDManager: Handling effect command");
    transitionTo(LEDManagerState::EFFECT_PLAYING);
    
    // For now, handle simple white LED effect
    String effectType = command["effect"]["type"];
    if (effectType == "white" || effectType == "wave_pattern" || effectType == "fire_pattern") {
        LOG_DEBUG("LEDManager: Playing white LED effect");
        // White LEDs will be rendered in render() method
    }
    else if (effectType == "idle") {
        LOG_DEBUG("LEDManager: Idle effect requested - staying in EFFECT_PLAYING state with white LEDs");
        // Stay in EFFECT_PLAYING state but render white LEDs
    }
    // TODO: Play other effects when EffectManager is built
    // if (effectManager) {
    //     effectManager->playEffect(command["effect"]);
    // }
}

void LEDManager::handleSequenceCommand(const JsonObject& command) {
    LOG_DEBUG("LEDManager: Handling sequence command");
    transitionTo(LEDManagerState::SEQUENCE_PLAYING);
    // TODO: Play sequence when SequenceManager is built
    // if (sequenceManager) {
    //     sequenceManager->playSequence(command["timeline"]);
    // }
}

void LEDManager::handleChoreographyCommand(const JsonObject& command) {
    LOG_DEBUG("LEDManager: Handling choreography command");
    transitionTo(LEDManagerState::CHOREOGRAPHY_PLAYING);
    // TODO: Play choreography when ChoreographyManager is built
    // if (choreographyManager) {
    //     choreographyManager->playChoreography(command);
    // }
}

void LEDManager::handleEmergencyCommand(const JsonObject& command) {
    LOG_DEBUG("LEDManager: Handling emergency command");
    pushState(LEDManagerState::EMERGENCY);
}

void LEDManager::setBrightness(int brightness) {
    // Clamp brightness to valid range
    brightness = constrain(brightness, 0, 255);
    
    if (brightness != currentBrightness) {
        currentBrightness = brightness;
        LOG_DEBUG("LEDManager: Brightness set to " + String(brightness));
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
    LOG_DEBUG("LEDManager: Pushing state: " + String((int)newState) + " (stack depth: " + String(stateStack.size()) + ")");
    
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
        LOG_WARN("LEDManager: Cannot pop state - stack is empty");
        return;
    }
    
    LEDManagerState currentState = getCurrentState();
    LOG_DEBUG("LEDManager: Popping state: " + String((int)currentState) + " (stack depth: " + String(stateStack.size()) + ")");
    
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
        LOG_WARN("LEDManager: Cannot pop state - stack is empty");
        return;
    }
    
    LEDManagerState currentState = getCurrentState();
    if (currentState != expectedState) {
        LOG_WARN("LEDManager: Expected to pop state " + String((int)expectedState) + " but current state is " + String((int)currentState) + " - ignoring");
        return;
    }
    
    LOG_DEBUG("LEDManager: Popping expected state: " + String((int)currentState) + " (stack depth: " + String(stateStack.size()) + ")");
    
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
    LOG_DEBUG("LEDManager: Replacing state with: " + String((int)newState));
    
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
