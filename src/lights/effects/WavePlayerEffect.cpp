#include "WavePlayerEffect.h"
#include "freertos/LogManager.h"

WavePlayerEffect::WavePlayerEffect(int id, WavePlayerConfig wavePlayerConfig)
    : Effect(id), wavePlayerConfig(wavePlayerConfig) {
    
    LOG_DEBUGF_COMPONENT("Effects", "WavePlayerEffect: Created with ID %d", id);
}

void WavePlayerEffect::initialize(Light *output, int numLEDs) {
    // for (auto &l : buffer) {
    //     l = Light(0, 0, 0);
    // }

/*
    for (auto &panel : lightPanels) {
        panel.init_Src(buffer, 32, 32);
        panel.type = 2;
    }

    lightPanels[0].set_SrcArea(16, 16, 0, 0);
    lightPanels[0].pTgt0 = output;
    lightPanels[0].rotIdx = 1;

    lightPanels[1].set_SrcArea(16, 16, 0, 16);
    lightPanels[1].pTgt0 = output + 256;
    lightPanels[1].rotIdx = -1;

    lightPanels[2].set_SrcArea(16, 16, 16, 0);
    lightPanels[2].pTgt0 = output + 512;
    lightPanels[2].rotIdx = 1;

    lightPanels[3].set_SrcArea(16, 16, 16, 16);
    lightPanels[3].pTgt0 = output + 768;
    lightPanels[3].rotIdx = 1;
    */
    // LOG_DEBUGF_COMPONENT("Effects", "WavePlayerEffect: Initializing light panels");
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
    // wavePlayer.setSeriesCoeffs(wavePlayerConfig.C_Rt, wavePlayerConfig.nTermsRt, wavePlayerConfig.C_Lt, wavePlayerConfig.nTermsLt);
    // wavePlayer.setSeriesCoeffs(C_Rt, wavePlayerConfig.nTermsRt, C_Lt, wavePlayerConfig.nTermsLt);
    wavePlayer.setWaveData(wavePlayerConfig.AmpRt, wavePlayerConfig.wvLenLt, wavePlayerConfig.wvSpdLt, wavePlayerConfig.wvLenRt, wavePlayerConfig.wvSpdRt);
    wavePlayer.setRightTrigFunc(wavePlayerConfig.rightTrigFuncIndex);
    wavePlayer.setLeftTrigFunc(wavePlayerConfig.leftTrigFuncIndex);
    // wavePlayer.AmpRt = wavePlayerConfig.AmpRt;
    // wavePlayer.AmpLt = wavePlayerConfig.AmpLt;
    // wavePlayer.wvLenLt = wavePlayerConfig.wvLenLt;
    // wavePlayer.wvLenRt = wavePlayerConfig.wvLenRt;
    // wavePlayer.wvSpdLt = wavePlayerConfig.wvSpdLt;
    // wavePlayer.wvSpdRt = wavePlayerConfig.wvSpdRt;

    update(0.f);
    LOG_DEBUGF_COMPONENT("Effects", "WavePlayerEffect: Initialized");
    isInitialized = true;
}
void WavePlayerEffect::update(float dt) {
    if (!isActive) return;
    if (!isInitialized) return;
    const auto speedFactor = wavePlayerConfig.speed;
    wavePlayer.update(0.16f * speedFactor);
}

void WavePlayerEffect::render(Light* output, int numLEDs) {
    if (!isActive) return;
    if (!isInitialized) {
        LOG_WARN_COMPONENT("Effects", "WavePlayerEffect: Not initialized, initializing...");
        initialize(output, numLEDs);
        update(0.f);
    }
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
