#ifndef WAVEPLAYER_H
#define WAVEPLAYER_H

// #include "Light.h"
#include "FastTrig.h"
#include <FastLED.h>

typedef CRGB Light;

using trig_func_t = float (*)(float);

struct WavePlayerConfig {
    int rows, cols;
    unsigned int rightTrigFuncIndex = 0;
    unsigned int leftTrigFuncIndex = 0;
    Light onLight, offLight;
    bool useRightCoefficients = false;
    bool useLeftCoefficients = false;
    float *C_Rt = nullptr;
    float *C_Lt = nullptr;
    unsigned int nTermsRt = 0;
    unsigned int nTermsLt = 0;
    float AmpLt, AmpRt;
    float wvLenLt, wvLenRt;
    float wvSpdLt, wvSpdRt;

    WavePlayerConfig() {}
    ~WavePlayerConfig() {}
    WavePlayerConfig(int rows,
        int cols,
        Light onLight,
        Light offLight,
        float AmpRt,
        float wvLenLt,
        float wvLenRt,
        float wvSpdLt,
        float wvSpdRt,
        unsigned int rightTrigFuncIndex,
        unsigned int leftTrigFuncIndex,
        bool useRightCoefficients,
        bool useLeftCoefficients,
        float *C_Rt,
        unsigned int numTermsRight,
        float *C_Lt, unsigned int numTermsLeft
    ) : rows(rows), cols(cols), onLight(onLight), offLight(offLight), AmpRt(AmpRt), wvLenLt(wvLenLt), wvLenRt(wvLenRt), wvSpdLt(wvSpdLt), wvSpdRt(wvSpdRt), rightTrigFuncIndex(rightTrigFuncIndex), leftTrigFuncIndex(leftTrigFuncIndex), useRightCoefficients(useRightCoefficients), useLeftCoefficients(useLeftCoefficients), C_Rt(C_Rt), nTermsRt(numTermsRight), C_Lt(C_Lt), nTermsLt(numTermsLeft)
    {}
};

class WavePlayer
{
public:
    // traveling wave to right
    float tElapRt = 0.0f, periodRt = 1.0f;// period = wvLen/Spd below
    float wvLenRt = 10.0f, wvSpdRt = 20.0f;// in array index
    float *C_Rt = nullptr;// provide external storage for Fourier coefficients
    unsigned int nTermsRt = 0;// in series
    float AmpRt = 1.0f;

    // traveling wave to left
    float tElapLt = 0.0f, periodLt = 1.0f;// actually = period = wvLen/Spd
    float wvLenLt = 10.0f, wvSpdLt = 20.0f;
    float *C_Lt = nullptr;// provide external storage
    unsigned int nTermsLt = 0;
    float AmpLt = 1.0f;// AmpLt = 1.0f - AmpRt

    Light hiLt, loLt;// interpolate
    float frHi = 0.0f, fgHi = 0.0f, fbHi = 0.0f;// store once
    float frLo = 0.0f, fgLo = 0.0f, fbLo = 0.0f;// used in update

    trig_func_t rightTrigFunc = sinf;
    trig_func_t leftTrigFunc = sinf;

    void update(float dt);

    void init(Light &r_Lt0, unsigned int Rows, unsigned int Cols, Light HiLt, Light LoLt);
    void setRightTrigFunc(unsigned int func);
    void setLeftTrigFunc(unsigned int func);
    void setWaveData(float AmpRt, float wvLen_lt, float wvSpd_lt, float wvLen_rt, float wvSpd_rt);
    void setSeriesCoeffs(float *C_rt, unsigned int n_TermsRt, float *C_lt, unsigned int n_TermsLt);
    void setSeriesCoeffs_Unsafe(float *C_rt, unsigned int n_TermsRt, float *C_lt, unsigned int n_TermsLt);

    WavePlayer() {}
    ~WavePlayer() {}

    Light *pLt0 = nullptr;
    unsigned int rows = 1, cols = 1;
    //    unsigned int iter = 0;// 0 to numLts
        // dependent
    unsigned int numLts = 1;

private:
};

// class WavePlayer
// {
//     public:
//     // traveling wave to right
//     float tElapRt = 0.0f, periodRt = 1.0f;// period = wvLen/Spd below
//     float wvLenRt = 10.0f, wvSpdRt = 20.0f;// in array index
//     float* C_Rt = nullptr;// provide external storage for Fourier coefficients
//     unsigned int nTermsRt = 0;// in series
//     float AmpRt = 1.0f;

//     // traveling wave to left
//     float tElapLt = 0.0f, periodLt = 1.0f;// actually = period = wvLen/Spd
//     float wvLenLt = 10.0f, wvSpdLt = 20.0f;
//     float* C_Lt = nullptr;// provide external storage
//     unsigned int nTermsLt = 0;
//     float AmpLt = 1.0f;// AmpLt = 1.0f - AmpRt

//     Light hiLt, loLt;// interpolate
//     float frHi = 0.0f, fgHi = 0.0f, fbHi = 0.0f;// store once
//     float frLo = 0.0f, fgLo = 0.0f, fbLo = 0.0f;// used in update

//     void update( float dt );

//     void init( Light& r_Lt0, unsigned int Rows, unsigned int Cols, Light HiLt, Light LoLt );
//     void setWaveData( float AmpRt, float wvLen_lt, float wvSpd_lt, float wvLen_rt, float wvSpd_rt );
//     void setSeriesCoeffs( float* C_rt, unsigned int n_TermsRt, float* C_lt, unsigned int n_TermsLt );

//     WavePlayer(){}
//     ~WavePlayer(){}

//     protected:
//     Light* pLt0 = nullptr;
//     unsigned int rows = 1, cols = 1;
// //    unsigned int iter = 0;// 0 to numLts
//     // dependent
//     unsigned int numLts = 1;

//     private:
// };

#endif // WAVEPLAYER_H
