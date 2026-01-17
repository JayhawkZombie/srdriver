#include "WavePlayerEffect.h"
#include "freertos/LogManager.h"

WavePlayerEffect::WavePlayerEffect(int id, WavePlayerConfig wavePlayerConfig)
    : Effect(id), wavePlayerConfig(wavePlayerConfig) {
    
    LOG_DEBUGF_COMPONENT("Effects", "WavePlayerEffect: Created with ID %d", id);
}

void WavePlayerEffect::initialize(Light* output, int numLEDs) {
    this->numLEDs = numLEDs;
    outputBuffer = output;
    
    LOG_DEBUGF_COMPONENT("WavePlayerEffect", "Initializing WavePlayer with output buffer and %d LEDs", numLEDs);
    wavePlayer.init(output[0], wavePlayerConfig.rows, wavePlayerConfig.cols, wavePlayerConfig.onLight, wavePlayerConfig.offLight);
    LOG_DEBUGF_COMPONENT("Effects", "WavePlayerEffect: colors set to %d, %d, %d", wavePlayerConfig.onLight.r, wavePlayerConfig.onLight.g, wavePlayerConfig.onLight.b);
    LOG_DEBUGF_COMPONENT("Effects", "WavePlayerEffect: colors set to %d, %d, %d", wavePlayerConfig.offLight.r, wavePlayerConfig.offLight.g, wavePlayerConfig.offLight.b);
    
    if (wavePlayerConfig.useRightCoefficients || wavePlayerConfig.useLeftCoefficients) {
        float *C_Rt = wavePlayerConfig.useRightCoefficients ? wavePlayerConfig.C_Rt : nullptr;
        float *C_Lt = wavePlayerConfig.useLeftCoefficients ? wavePlayerConfig.C_Lt : nullptr;
        unsigned int nTermsRt = wavePlayerConfig.useRightCoefficients ? wavePlayerConfig.nTermsRt : 0;
        unsigned int nTermsLt = wavePlayerConfig.useLeftCoefficients ? wavePlayerConfig.nTermsLt : 0;
        wavePlayer.setSeriesCoeffs_Unsafe(C_Rt, nTermsRt, C_Lt, nTermsLt);
    }
    wavePlayer.setWaveData(wavePlayerConfig.AmpRt, wavePlayerConfig.wvLenLt, wavePlayerConfig.wvSpdLt, wavePlayerConfig.wvLenRt, wavePlayerConfig.wvSpdRt);
    wavePlayer.setRightTrigFunc(wavePlayerConfig.rightTrigFuncIndex);
    wavePlayer.setLeftTrigFunc(wavePlayerConfig.leftTrigFuncIndex);
    
    isInitialized = true;
    LOG_DEBUGF_COMPONENT("Effects", "WavePlayerEffect: WavePlayer initialized");
}

void WavePlayerEffect::update(float dt) {
    if (!isActive) return;
    if (!isInitialized) return;
    const auto speedFactor = wavePlayerConfig.speed;
    wavePlayer.update(dt * speedFactor);
}

void WavePlayerEffect::render(Light* output) {
    if (!isActive) return;
    // WavePlayer writes directly to the buffer via update(), nothing to do here
    // lightPanels[0].pTgt0 = output;
    // lightPanels[1].pTgt0 = output + 256;
    // lightPanels[2].pTgt0 = output + 512;
    // lightPanels[3].pTgt0 = output + 768;

    // buffer[0] = Light(255, 255, 255);
    // buffer[16] = Light(255, 255, 255);
    // buffer[16 * 32] = Light(255, 255, 255);
    // buffer[16 * 32 + 16] = Light(255, 255, 255);

    // for (auto &panel : lightPanels) {
    //     panel.update();
    // }
}
