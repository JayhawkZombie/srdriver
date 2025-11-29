#pragma once
#include "Effect.h"
#include "../Light.h"

class SolidColorEffect : public Effect {
public:
    SolidColorEffect(int id, const String& colorString, float duration = -1.0f);
    
    void update(float dt) override;
    void render(Light* output, int numLEDs) override;
    bool isFinished() const override;

private:
    int red, green, blue;
    float duration;
    float elapsed;
    bool hasDuration;
    
    void parseColorString(const String& colorString);
};
