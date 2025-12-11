#include "RainEffect.h"
#include "freertos/LogManager.h"

RainEffect::RainEffect(int id)
    : Effect(id)
{
    isInitialized = false;
}

void RainEffect::initialize(Light *output, int numLEDs)
{
    // Clear buffer
    // for (auto &l : buffer)
    // {
    //     l = Light(0, 0, 0);
    // }

    for (auto &rp : ringPlayers) {
        rp.initToGrid(output, 32, 32);
    }

    // ringPlayers[0].setRingProps(10.f, 2.f, 10.f, 2.f);
    // ringPlayers[0].setRingCenter(16.f, 16.f);

    // for (auto &panel : lightPanels) {
    //     panel.init_Src(buffer, 32, 32);
    //     panel.type = 2;
    // }

    // lightPanels[0].set_SrcArea(16, 16, 0, 0);
    // lightPanels[0].pTgt0 = output;
    // lightPanels[0].rotIdx = 1;

    // lightPanels[1].set_SrcArea(16, 16, 0, 16);
    // lightPanels[1].pTgt0 = output + 256;
    // lightPanels[1].rotIdx = -1;

    // lightPanels[2].set_SrcArea(16, 16, 16, 0);
    // lightPanels[2].pTgt0 = output + 512;
    // lightPanels[2].rotIdx = 1;
    
    // lightPanels[3].set_SrcArea(16, 16, 16, 16);
    // lightPanels[3].pTgt0 = output + 768;
    // lightPanels[3].rotIdx = 1;

    isInitialized = true;
}

void RainEffect::update(float dt)
{
    if (!isActive) return;
    if (!isInitialized) { return; }


    static unsigned long lastTime = micros();
    unsigned long currTime = micros();
    // dtLoop = (currTime - lastTime) * 0.000001f;
    lastTime = currTime;

    // for (auto &l : buffer)
    // {
    //     l = Light(0, 0, 0);
    // }


    // Update the ring players
    numRpPlaying = 0;

    for (unsigned int k = 0; k < ringPlayers.size(); ++k)
    {
        ringPlayers[k].update(dt);
        if (!ringPlayers[k].onePulse && ringPlayers[k].isRadiating)
        {
            float R = ringPlayers[k].ringSpeed * ringPlayers[k].tElap;
            if (R > 3.0f * ringPlayers[k].ringWidth)
                ringPlayers[k].StopWave();
        }

        if (ringPlayers[k].isPlaying) ++numRpPlaying;
    }

    // Start another
    tElapStart += dt;
    if (tElapStart >= tStart)
    {
        tElapStart = 0.0f;
        RingPlayer &RP = ringPlayers[idxStartNext];
        //  int idxRD = rand()%numRingDataInUse;
        //  RP.setup( ringData[idxRD] );
        int rC = spawnRowRange.random(), cC = spawnColumnRange.random();
        RP.setRingCenter(rC, cC);
        RP.hiLt.r = hiLightRange.random();
        RP.hiLt.g = hiLightRange.random();
        RP.hiLt.b = hiLightRange.random();
        RP.loLt.r = loLightRange.random();
        RP.loLt.g = loLightRange.random();
        RP.loLt.b = loLightRange.random();
        // ring props
        if ((rand() % oddsOfRadiating) == 1) RP.onePulse = false;// 1 in 3 chance to radiate
        else RP.onePulse = true;

        float ringWidth = ringWidthRange.random();
        if (!RP.onePulse) ringWidth *= 0.6f;
        float fadeR = fadeRratio * ringWidth;
        float fadeW = fadeWratio * ringWidth;
        float time = lifetimeRange.random();
        float Speed = speedFactor * (fadeR + fadeW) / time;
        RP.setRingProps(Speed, ringWidth, fadeR, fadeW);
        RP.Amp = amplitudeRange.random();

        RP.Start();
        idxStartNext = (1 + idxStartNext) % ringPlayers.size();
        tStart = tStartFactor * spawnTime * (float) (rand() % tStartMod);
        tStart /= (tStartMod * 1.f);
    }



    // if (!ringPlayers[0].isPlaying) {
    //     ringPlayers[0].Start();
    // }

    // for (auto &rp : ringPlayers)
    // {
    //     rp.update(dt);
    // }

    // buffer[0] = Light(255, 0, 0);
    // buffer[16] = Light(255, 0, 0);
    // buffer[32 * 16] = Light(255, 0, 0);
    // buffer[32 * 16 + 16] = Light(255, 0, 0);
}

void RainEffect::render(Light *output, int numLEDs)
{
    if (!isInitialized) {
        LOG_WARN_COMPONENT("Effects", "RainEffect: Not initialized, initializing...");
        initialize(output, numLEDs);
    }

    // lightPanels[0].pTgt0 = output;
    // lightPanels[1].pTgt0 = output + 256;
    // lightPanels[2].pTgt0 = output + 512;
    // lightPanels[3].pTgt0 = output + 768;

    // for (auto &panel : lightPanels) {
    //     panel.update();
    // }
}

bool RainEffect::isFinished() const
{
    return false;
}