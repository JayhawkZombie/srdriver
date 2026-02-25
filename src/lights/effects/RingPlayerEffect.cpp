#include "RingPlayerEffect.h"
#include "freertos/LogManager.h"

RingPlayerEffect::RingPlayerEffect(int id, RPdata ringPlayerConfig)
    : Effect(id)
{
    this->ringPlayerConfig = ringPlayerConfig;
    isInitialized = false;
}

void RingPlayerEffect::initialize(Light *output, int numLEDs)
{
    this->numLEDs = numLEDs;
    this->outputBuffer = output;
    ringPlayer.initToGrid(output, 32, 32);
    ringPlayer.setup(ringPlayerConfig);
    ringPlayer.onePulse = false;
    LOG_DEBUGF_COMPONENT("RingPlayerEffect", "Ring player initialized");
    isInitialized = true;
    ringPlayer.Start();
}

bool RingPlayerEffect::updateParams(const JsonObject &params)
{
    bool didUpdate = false;
    if (params.containsKey("ringWidth"))
    {
        ringPlayerConfig.ringWidth = params["ringWidth"].as<float>();
        didUpdate = true;
    }


    if (didUpdate)
    {
        ringPlayer.setup(ringPlayerConfig);
        ringPlayer.Start();
    }
    return didUpdate;
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
    if (!isInitialized) return;
}