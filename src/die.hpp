#include <FastLED.h>

enum class CauseOfDeath : uint8_t
{
    Nullptr = 0,
    Memory,
    Logic,
    SDCardInitFailed,
    SDCardReadFailed,
    __NumCauses,
};

CRGB errorColors[(unsigned long)CauseOfDeath::__NumCauses] = {
    CRGB::Red,
    CRGB::Blue,
    CRGB::Purple
};

[[noreturn]] void die(CRGB *leds, CauseOfDeath reason)
{
    CRGB color = errorColors[(unsigned long) reason];
    int mod = 1;
    while (true) {
        for (int i = 0; i < 10; ++i) {
            leds[i] = i % 2 == 0 ? color : CRGB::Black;
        }
        mod = (mod + 1) % 9 + 1;
        delay(500);
    }
        
}
