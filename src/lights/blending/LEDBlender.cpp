#include "LEDBlender.h"
#include <algorithm>
#include <cmath>

Light MultiplyBlender::operator()(const Light& a, const Light& b, float blendFactor) {
    return Light(
        (uint8_t)(a.r * blendFactor),
        (uint8_t)(a.g * blendFactor),
        (uint8_t)(a.b * blendFactor)
    );
}

Light AddBlender::operator()(const Light& a, const Light& b, float blendFactor) {
    return Light(
        std::min(255, a.r + (int)(b.r * blendFactor)),
        std::min(255, a.g + (int)(b.g * blendFactor)),
        std::min(255, a.b + (int)(b.b * blendFactor))
    );
}

Light ScreenBlender::operator()(const Light& a, const Light& b, float blendFactor) {
    return Light(
        (uint8_t)(255 - ((255 - a.r) * (255 - (int)(b.r * blendFactor)) / 255)),
        (uint8_t)(255 - ((255 - a.g) * (255 - (int)(b.g * blendFactor)) / 255)),
        (uint8_t)(255 - ((255 - a.b) * (255 - (int)(b.b * blendFactor)) / 255))
    );
}

Light HSVContrastBlender::operator()(const Light& a, const Light& b, float blendFactor) {
    // Convert background color to HSV using FastLED
    CRGB rgb(a.r, a.g, a.b);
    CHSV hsv = rgb2hsv_approximate(rgb);
    
    // Shift hue by 128 (180 degrees in FastLED's 0-255 range)
    hsv.hue += 128;
    
    // Convert back to RGB
    CRGB contrastRgb;
    contrastRgb.setHSV(hsv.hue, hsv.sat, hsv.val);
    
    // Blend between original and contrast color based on blendFactor
    return Light(
        (uint8_t)(a.r * (1.0f - blendFactor) + contrastRgb.r * blendFactor),
        (uint8_t)(a.g * (1.0f - blendFactor) + contrastRgb.g * blendFactor),
        (uint8_t)(a.b * (1.0f - blendFactor) + contrastRgb.b * blendFactor)
    );
}

Light SelectiveMaskBlender::operator()(const Light& a, const Light& b, float blendFactor) {
    // Use the red channel of the mask as intensity (like the original code)
    float maskIntensity = b.r / 255.0f;
    return Light(
        (uint8_t)(a.r * maskIntensity),
        (uint8_t)(a.g * maskIntensity),
        (uint8_t)(a.b * maskIntensity)
    );
}
