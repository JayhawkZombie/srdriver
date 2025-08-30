#pragma once

#include "../Light.h"
#include "LEDBlender.h"

class Layer {
public:
    virtual void update(float dt) = 0;
    virtual void render(Light* output, int numLeds) = 0;
    virtual bool isEnabled() const { return enabled; }
    virtual void setEnabled(bool e) { enabled = e; }
    virtual LEDBlender* getBlender() { return blender; }
    virtual void setBlender(LEDBlender* b) { blender = b; }
    virtual ~Layer() = default;

protected:
    bool enabled = true;
    LEDBlender* blender = nullptr;
}; 