#pragma once

#include <vector>
#include <memory>
#include "../Light.h"

class Layer;

class LayerStack {
private:
    std::vector<std::unique_ptr<Layer>> layers;
    Light* tempBuffer;
    int numLeds;

public:
    LayerStack(int ledCount);
    ~LayerStack();
    
    template<typename T, typename... Args>
    void addLayer(Args&&... args) {
        layers.push_back(std::unique_ptr<T>(new T(std::forward<Args>(args)...)));
    }
    
    void update(float dt);
    void render(Light* output);
};
