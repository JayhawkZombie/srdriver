#pragma once

#include "../Light.h"

class Layer {
public:
    virtual void update(float dt) = 0;
    virtual void render(Light* output, int numLeds) = 0;
    virtual bool isEnabled() const { return enabled; }
    virtual void setEnabled(bool e) { enabled = e; }
    virtual ~Layer() = default;

protected:
    bool enabled = true;
}; 