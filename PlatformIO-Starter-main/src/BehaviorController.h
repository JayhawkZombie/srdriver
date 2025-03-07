#include <FastLED.h>

class BehaviorController {
    public:
        BehaviorController() = default;
        BehaviorController(CRGB *leds[30]) : m_leds(leds) {}
        ~BehaviorController() = default;

        virtual void Update(unsigned long tick) = 0;
        virtual void Display();

        virtual void Start() = 0;

        void SetLEDColor(uint16_t index, const CRGB &color) {
            (*m_leds)[index] = color;
        }
        const CRGB& GetLEDColor(uint16_t index) {
            return (*m_leds)[index];
        }

    private:

        CRGB **m_leds = nullptr;
};
