#pragma once

#include "../Light.h"
#include <FastLED.h>

struct LEDBlender {
    virtual Light operator()(const Light& a, const Light& b, float blendFactor) = 0;
    virtual ~LEDBlender() = default;
};

struct MultiplyBlender : public LEDBlender {
    Light operator()(const Light& a, const Light& b, float blendFactor) override;
};

struct AddBlender : public LEDBlender {
    Light operator()(const Light& a, const Light& b, float blendFactor) override;
};

struct ScreenBlender : public LEDBlender {
    Light operator()(const Light& a, const Light& b, float blendFactor) override;
};

struct HSVContrastBlender : public LEDBlender {
    Light operator()(const Light& a, const Light& b, float blendFactor) override;
};


