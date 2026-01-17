#include "EffectManager.h"
#include "freertos/LogManager.h"

EffectManager::EffectManager() : nextEffectId(1) {
    LOG_DEBUG("EffectManager: Initializing");
}

EffectManager::~EffectManager() {
    LOG_DEBUG("EffectManager: Destroying");
    removeAllEffects();
}

void EffectManager::addEffect(std::unique_ptr<Effect> effect, Light* output, int numLEDs) {
    if (!effect) {
        LOG_ERROR("EffectManager: Cannot add null effect");
        return;
    }
    
    int effectId = effect->getId();
    LOG_DEBUG("EffectManager: Adding effect with ID " + String(effectId));
    
    // Check if effect with same ID already exists
    if (hasEffect(effectId)) {
        LOG_WARN("EffectManager: Effect with ID " + String(effectId) + " already exists, removing old one");
        removeEffect(effectId);
    }
    
    // Initialize the effect with output buffer and numLEDs before adding
    effect->initialize(output, numLEDs);
    effect->start();
    activeEffects.push_back(std::move(effect));
    LOG_DEBUG("EffectManager: Effect added, total active effects: " + String(activeEffects.size()));
}

void EffectManager::removeEffect(int effectId) {
    LOG_DEBUG("EffectManager: Removing effect with ID " + String(effectId));
    
    auto it = std::find_if(activeEffects.begin(), activeEffects.end(),
        [effectId](const std::unique_ptr<Effect>& effect) {
            return effect->getId() == effectId;
        });
    
    if (it != activeEffects.end()) {
        (*it)->stop();
        activeEffects.erase(it);
        LOG_DEBUG("EffectManager: Effect removed, total active effects: " + String(activeEffects.size()));
    } else {
        LOG_WARN("EffectManager: Effect with ID " + String(effectId) + " not found");
    }
}

void EffectManager::removeAllEffects() {
    LOG_DEBUG("EffectManager: Removing all effects");
    
    for (auto& effect : activeEffects) {
        effect->stop();
    }
    activeEffects.clear();
    
    LOG_DEBUG("EffectManager: All effects removed");
}

void EffectManager::update(float dt) {
    // Update all active effects
    for (auto& effect : activeEffects) {
        if (effect->getIsActive()) {
            effect->update(dt);
        }
    }
    
    // Clean up finished effects
    cleanupFinishedEffects();
}

void EffectManager::render(Light* output) {
    // Debug logging
    static int debugCounter = 0;
    if (debugCounter++ % 100 == 0) {
        LOG_DEBUGF_COMPONENT("EffectManager", "Rendering %d active effects", activeEffects.size());
    }
    
    // Render all active effects (blending)
    for (auto& effect : activeEffects) {
        if (effect->getIsActive()) {
            effect->render(output);
        }
    }
}

bool EffectManager::hasEffect(int effectId) const {
    return std::any_of(activeEffects.begin(), activeEffects.end(),
        [effectId](const std::unique_ptr<Effect>& effect) {
            return effect->getId() == effectId;
        });
}

Effect* EffectManager::getEffect(int effectId) {
    auto it = std::find_if(activeEffects.begin(), activeEffects.end(),
        [effectId](const std::unique_ptr<Effect>& effect) {
            return effect->getId() == effectId;
        });
    
    return (it != activeEffects.end()) ? it->get() : nullptr;
}

void EffectManager::pauseEffect(int effectId) {
    Effect* effect = getEffect(effectId);
    if (effect) {
        effect->pause();
        LOG_DEBUG("EffectManager: Paused effect with ID " + String(effectId));
    } else {
        LOG_WARN("EffectManager: Cannot pause effect with ID " + String(effectId) + " - not found");
    }
}

void EffectManager::resumeEffect(int effectId) {
    Effect* effect = getEffect(effectId);
    if (effect) {
        effect->resume();
        LOG_DEBUG("EffectManager: Resumed effect with ID " + String(effectId));
    } else {
        LOG_WARN("EffectManager: Cannot resume effect with ID " + String(effectId) + " - not found");
    }
}

void EffectManager::stopEffect(int effectId) {
    Effect* effect = getEffect(effectId);
    if (effect) {
        effect->stop();
        LOG_DEBUG("EffectManager: Stopped effect with ID " + String(effectId));
    } else {
        LOG_WARN("EffectManager: Cannot stop effect with ID " + String(effectId) + " - not found");
    }
}

void EffectManager::cleanupFinishedEffects() {
    auto it = std::remove_if(activeEffects.begin(), activeEffects.end(),
        [](const std::unique_ptr<Effect>& effect) {
            return effect->isFinished();
        });
    
    if (it != activeEffects.end()) {
        int removedCount = std::distance(it, activeEffects.end());
        LOG_DEBUG("EffectManager: Cleaning up " + String(removedCount) + " finished effects");
        activeEffects.erase(it, activeEffects.end());
    }
}

int EffectManager::generateEffectId() {
    return nextEffectId++;
}
