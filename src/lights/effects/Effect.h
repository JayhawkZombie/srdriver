#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../Light.h"

/**
 * Base class for all LED effects
 * 
 * Provides the interface that all effects must implement:
 * - update() - called every frame to update effect state
 * - render() - called every frame to render effect to LED buffer
 * - isFinished() - indicates if effect should be removed
 * - getId() - unique identifier for effect management
 */
class Effect {
public:
    Effect(int id) : effectId(id), isActive(true) {}
    virtual ~Effect() = default;
    
    // Core effect interface
    virtual void update(float dt) = 0;
    virtual void initialize(Light* output, int numLEDs) = 0;
    virtual void render(Light* output) = 0;
    virtual bool isFinished() const = 0;
    
    // Effect management
    int getId() const { return effectId; }
    bool getIsActive() const { return isActive; }
    void setActive(bool active) { isActive = active; }
    
    // Effect lifecycle
    virtual void start() { isActive = true; }
    virtual void stop() { isActive = false; }
    virtual void pause() { isActive = false; }
    virtual void resume() { isActive = true; }

    // Optional: update parameters at runtime (e.g. from timeline). Returns true if params were applied.
    virtual bool updateParams(const JsonObject& params) { (void)params; return false; }

protected:
    int effectId;
    bool isActive;
};
