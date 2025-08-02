// #include "RainbowPlayer.h"
// #include "PatternManager.h"
// #include <FastLED.h>

// // Example usage of RainbowPlayer
// // This shows how to integrate it into the existing pattern system

// // Global rainbow player instance
// RainbowPlayer rainbowPlayer(nullptr, 0, 0, 0, 0.5f); // 0.5 rotations per second

// // Example function to initialize rainbow player
// void InitializeRainbowPlayer(Light* leds, int numLEDs)
// {
//     // Create rainbow effect on all LEDs
//     rainbowPlayer = RainbowPlayer(leds, numLEDs, 0, numLEDs - 1, 0.5f);
// }

// // Example function to update rainbow player (call this in your main update loop)
// void UpdateRainbowPlayer(float dtSeconds)
// {
//     rainbowPlayer.update(dtSeconds);
// }

// // Example function to change rainbow speed
// void SetRainbowSpeed(float speed)
// {
//     rainbowPlayer.setSpeed(speed);
// }

// // Example function to set rainbow to specific hue (0-255)
// void SetRainbowHue(uint8_t hue)
// {
//     rainbowPlayer.setHue(hue);
// }

// // Example function to create rainbow on specific LED range
// void CreateRainbowOnRange(Light* leds, int numLEDs, int startLED, int endLED, float speed)
// {
//     RainbowPlayer rangeRainbow(leds, numLEDs, startLED, endLED, speed);
//     // You would typically store this in a global variable or class member
//     // and call update() on it in your main loop
// }

// // Example integration with existing pattern system
// // This could be added to PatternManager.cpp
// void UpdateRainbowPattern(float dtSeconds)
// {
//     // Clear all LEDs first
//     for (int i = 0; i < NUM_LEDS; ++i)
//     {
//         LightArr[i].r = 0;
//         LightArr[i].g = 0;
//         LightArr[i].b = 0;
//     }
    
//     // Update rainbow player
//     rainbowPlayer.update(dtSeconds);
    
//     // Copy to FastLED array
//     for (int i = 0; i < NUM_LEDS; ++i)
//     {
//         leds[i].r = LightArr[i].r;
//         leds[i].g = LightArr[i].g;
//         leds[i].b = LightArr[i].b;
//     }
// }

// // Example of creating multiple rainbow players for different sections
// void CreateMultiSectionRainbow(Light* leds, int numLEDs)
// {
//     // Create rainbow on first third
//     RainbowPlayer section1(leds, numLEDs, 0, numLEDs/3 - 1, 0.3f);
    
//     // Create rainbow on middle third (different speed)
//     RainbowPlayer section2(leds, numLEDs, numLEDs/3, 2*numLEDs/3 - 1, 0.6f);
    
//     // Create rainbow on last third (reverse direction)
//     RainbowPlayer section3(leds, numLEDs, 2*numLEDs/3, numLEDs - 1, -0.4f);
    
//     // You would store these in global variables or class members
//     // and update them in your main loop
// }

// // Example showing how to use FastLED's native HSV system directly
// // This is what the RainbowPlayer does internally
// void DirectFastLEDRainbowExample(CRGB* leds, int numLEDs, uint8_t baseHue)
// {
//     // FastLED's native approach - very efficient
//     for (int i = 0; i < numLEDs; i++) {
//         // Each LED gets a hue offset of 10 units
//         uint8_t hue = baseHue + (i * 10);
//         leds[i] = CHSV(hue, 255, 255); // Full saturation and value
//     }
// }

// // Example showing how to convert between our Light class and FastLED's CRGB
// void ConvertLightToCRGB(Light* lightArray, CRGB* crgbArray, int numLEDs)
// {
//     for (int i = 0; i < numLEDs; i++) {
//         crgbArray[i] = CRGB(lightArray[i].r, lightArray[i].g, lightArray[i].b);
//     }
// } 