#include "RingPlayerEffect.h"

RingPlayerEffect::RingPlayerEffect(int id, RPdata ringPlayerConfig)
    : Effect(id)
{
    this->ringPlayerConfig = ringPlayerConfig;
}

void RingPlayerEffect::initialize(Light* output, int numLEDs)
{
    this->numLEDs = numLEDs;
    this->outputBuffer = output;
}

void RingPlayerEffect::update(float dt)
{
    if (!isActive) return;
    if (!isInitialized) { return; }

    ringPlayer.update(dt);
}

void RingPlayerEffect::render(Light *output)
{
    if (!isActive) return;
}