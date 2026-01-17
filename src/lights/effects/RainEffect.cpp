#include "RainEffect.h"
#include "freertos/LogManager.h"

RainEffect::RainEffect(int id)
    : Effect(id)
{
    isInitialized = false;
}

void RainEffect::initialize(Light* output, int numLEDs)
{
    this->numLEDs = numLEDs;
    outputBuffer = output;
    
    LOG_DEBUGF_COMPONENT("RainEffect", "Initializing RingPlayers with output buffer and %d LEDs", numLEDs);
    for (auto &rp : ringPlayers) {
        rp.initToGrid(output, 32, 32);
    }
    isInitialized = true;
    LOG_DEBUGF_COMPONENT("RainEffect", "RingPlayers initialized");
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

        const auto hsvHiColor = CHSV(hiLightRange.random(), 255, 255);
        CRGB hiColorRGB;
        hsv2rgb_raw(hsvHiColor, hiColorRGB);

        RP.hiLt = hiColorRGB;
        const auto hsvLoColor = CHSV(loLightRange.random(), 255, 255);
        CRGB loColorRGB;
        hsv2rgb_raw(hsvLoColor, loColorRGB);
        RP.loLt = loColorRGB;
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

void RainEffect::render(Light *output)
{
    if (!isActive) return;
    // RingPlayers write directly to the buffer via update(), nothing to do here

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