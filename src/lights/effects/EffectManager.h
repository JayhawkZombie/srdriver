#pragma once

#include <Arduino.h>
#include <memory>
#include <vector>
#include <algorithm>
#include "Effect.h"
#include "../LightPanel.h"
#include "../Light.h"
#include "../../Globals.h"
/**
 * Manages multiple running LED effects
 * 
 * Handles:
 * - Adding/removing effects
 * - Updating all active effects
 * - Blending multiple effects together
 * - Effect lifecycle management
 */
class EffectManager {
public:
    EffectManager();
    ~EffectManager();
    
    // Effect management
    void addEffect(std::unique_ptr<Effect> effect, Light* output, int numLEDs);
    void removeEffect(int effectId);
    void removeAllEffects();
    
    // Update and render
    void update(float dt);
    void render(Light* output);
    
    // Effect queries
    int getActiveEffectCount() const { return activeEffects.size(); }
    bool hasEffect(int effectId) const;
    Effect* getEffect(int effectId);
    
    // Effect control
    void pauseEffect(int effectId);
    void resumeEffect(int effectId);
    void stopEffect(int effectId);
    
private:
    std::vector<std::unique_ptr<Effect>> activeEffects;
    int nextEffectId;
    
    // Helper methods
    void cleanupFinishedEffects();
    int generateEffectId();
};
