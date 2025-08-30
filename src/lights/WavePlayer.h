#ifndef WAVEPLAYER_H
#define WAVEPLAYER_H

#include "Light.h"
#include "FastTrig.h"
#include <FastLED.h>
#include <freertos/LogManager.h>

using trig_func_t = float (*)(float);

struct WavePlayerConfig {
    int rows, cols;
    unsigned int rightTrigFuncIndex = 0;
    unsigned int leftTrigFuncIndex = 0;
    Light onLight, offLight;
    bool useRightCoefficients = false;
    bool useLeftCoefficients = false;
    // float *C_Rt = nullptr;
    // float *C_Lt = nullptr;
    float C_Rt[3] = {0, 0, 0};
    String name = "Wave Player";
    float C_Lt[3] = {0, 0, 0};
    unsigned int nTermsRt = 0;
    unsigned int nTermsLt = 0;
    float AmpLt, AmpRt;
    float speed = 0.01f;
    float wvLenLt, wvLenRt;
    float wvSpdLt, wvSpdRt;

    WavePlayerConfig() {}
    ~WavePlayerConfig() {}
    // WavePlayerConfig(const WavePlayerConfig &other)
    // {
    //     *this = other;
    // }
    // WavePlayerConfig &operator=(const WavePlayerConfig &other)
    // {
    //     rows = other.rows;
    //     cols = other.cols;
    //     rightTrigFuncIndex = other.rightTrigFuncIndex;
    //     leftTrigFuncIndex = other.leftTrigFuncIndex;
    //     onLight = other.onLight;
    //     offLight = other.offLight;
    //     useRightCoefficients = other.useRightCoefficients;
    //     useLeftCoefficients = other.useLeftCoefficients;
    //     nTermsRt = other.nTermsRt;
    //     nTermsLt = other.nTermsLt;
    //     AmpRt = other.AmpRt;
    //     name = other.name;
    //     wvLenLt = other.wvLenLt;
    //     wvLenRt = other.wvLenRt;
    //     wvSpdLt = other.wvSpdLt;
    //     wvSpdRt = other.wvSpdRt;
    //     C_Rt[0] = other.C_Rt[0];
    //     C_Rt[1] = other.C_Rt[1];
    //     C_Rt[2] = other.C_Rt[2];
    //     C_Lt[0] = other.C_Lt[0];
    //     C_Lt[1] = other.C_Lt[1];
    //     C_Lt[2] = other.C_Lt[2];
    //     return *this;
    // }
    // WavePlayerConfig &operator=(WavePlayerConfig &&other)
    // {
    //     *this = other;
    //     return *this;
    // }
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
    ) : rows(rows), cols(cols), onLight(onLight), offLight(offLight), AmpRt(AmpRt), wvLenLt(wvLenLt), wvLenRt(wvLenRt), wvSpdLt(wvSpdLt), wvSpdRt(wvSpdRt), rightTrigFuncIndex(rightTrigFuncIndex), leftTrigFuncIndex(leftTrigFuncIndex), useRightCoefficients(useRightCoefficients), useLeftCoefficients(useLeftCoefficients), nTermsRt(numTermsRight), nTermsLt(numTermsLeft)
    {
        setCoefficients(C_Rt, C_Lt);
    }

    void setCoefficients(float *C_Rt, float *C_Lt)
    {
        for (int i = 0; i < 3; i++)
        {
            this->C_Rt[i] = C_Rt ? C_Rt[i] : 0;
            this->C_Lt[i] = C_Lt ? C_Lt[i] : 0;
        }
    }
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
