#include "BehaviorController.h"

class BlinkerController : public BehaviorController {
public:

    BlinkerController(CRGB *leds[30],
        unsigned long blinkInterval,
        unsigned long blinkDelay,
        uint16_t lightIndex
    )
        : BehaviorController(leds),
        interval(blinkInterval),
        delay(blinkDelay),
        index(lightIndex)
        {}

    unsigned long interval;
    unsigned long delay;
    unsigned long elapsed;
    uint16_t index;
    CRGB lastColor;
    bool isOn = false;

    void Start() override {
        elapsed = 0;
    }
    

    void  Update(unsigned long tick) override {
        elapsed += tick;
        if (isOn && elapsed >= interval) {
            isOn = false;
            lastColor = GetLEDColor(index);
            SetLEDColor(index, CRGB::Black);
        } else if (!isOn && elapsed >= delay) {
            isOn = true;
            SetLEDColor(index, lastColor);
        }
    }

    void Display() override {
        
    }

};
