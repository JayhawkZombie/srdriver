#include "WavePlayer.h"

void WavePlayer::init(Light &r_Lt0, unsigned int Rows, unsigned int Cols, Light HiLt, Light LoLt)
{
    pLt0 = &r_Lt0;
    rows = Rows;
    cols = Cols;
    numLts = rows * cols;
    hiLt = HiLt;
    loLt = LoLt;
    tElapLt = tElapRt = 0.0f;

    frHi = (float) hiLt.r; fgHi = (float) hiLt.g; fbHi = (float) hiLt.b;
    frLo = (float) loLt.r; fgLo = (float) loLt.g; fbLo = (float) loLt.b;

    // check
    unsigned int rd = HiLt.r, gn = HiLt.g, bu = HiLt.g;
    // std::cout << "\nwvPlay.init() HiLt.r = " << rd << " HiLt.g = " << gn << " HiLt.b = " << bu;
    rd = LoLt.r; gn = LoLt.g; bu = LoLt.b;
    // std::cout << "\n LoLt.r = " << rd << " LoLt.g = " << gn << " LoLt.b = " << bu;
}

void WavePlayer::setRightTrigFunc(unsigned int func)
{
    if (func == 0)
    {
        rightTrigFunc = sinf;
    }
    else if (func == 1)
    {
        rightTrigFunc = cosf;
    }
    else if (func == 2)
    {
        rightTrigFunc = tanf;
    }
    else if (func == 3)
    {
        rightTrigFunc = [](float x) -> float
            {
                return hypotf(x, 0.0f);
            };
    }
    else if (func == 4)
    {
        rightTrigFunc = sinh;
    }
    else if (func == 5)
    {
        rightTrigFunc = cosh;
    }
    else if (func == 6)
    {
        rightTrigFunc = tanh;
    }
}

void WavePlayer::setLeftTrigFunc(unsigned int func)
{
    if (func == 0)
    {
        leftTrigFunc = sinf;
    }
    else if (func == 1)
    {
        leftTrigFunc = cosf;
    }
    else if (func == 2)
    {
        leftTrigFunc = tanf;
    }
    else if (func == 3)
    {
        leftTrigFunc = [](float x) -> float
            {
                return hypotf(x, 0.0f);
            };
    }
    else if (func == 4)
    {
        leftTrigFunc = sinh;
    }
    else if (func == 5)
    {
        leftTrigFunc = cosh;
    }
    else if (func == 6)
    {
        leftTrigFunc = tanh;
    }
}

void WavePlayer::setWaveData(float ampRt, float wvLen_lt, float wvSpd_lt, float wvLen_rt, float wvSpd_rt)
{
    wvLenLt = wvLen_lt;
    wvLenRt = wvLen_rt;// in array indices
    wvSpdLt = wvSpd_lt;
    wvSpdRt = wvSpd_rt;
    AmpRt = ampRt;
    AmpLt = 1.0f - AmpRt;
    periodLt = wvLenLt / wvSpdLt;
    periodRt = wvLenRt / wvSpdRt;
    tElapLt = 0.0f;
    tElapRt = 0.0f;
}

void WavePlayer::setSeriesCoeffs(float *C_rt, unsigned int n_TermsRt, float *C_lt, unsigned int n_TermsLt)
{
    // std::cout << "WavePlayer::setSeriesCoeffs" << std::endl;
    C_Rt = C_rt;
    nTermsRt = n_TermsRt;
    C_Lt = C_lt;
    nTermsLt = n_TermsLt;

    // coefficients must sum to 1.0f
    float sum = 0.0f;
    if (C_Rt)
    {
        for (unsigned int k = 0; k < nTermsRt; ++k)
        {
            if (C_Rt[k] > 0.0f) sum += C_Rt[k];
            else sum -= C_Rt[k];
        }
        for (unsigned int k = 0; k < nTermsRt; ++k)
        {
            C_Rt[k] /= sum;
            // std::cout << "\nC_Rt[" << k << "] = " << C_Rt[k];
        }
    }

    if (C_Lt)
    {
        sum = 0.0f;
        for (unsigned int k = 0; k < nTermsLt; ++k)
        {
            if (C_Lt[k] > 0.0f) sum += C_Lt[k];
            else sum -= C_Lt[k];
        }
        for (unsigned int k = 0; k < nTermsLt; ++k)
        {
            C_Lt[k] /= sum;
            // std::cout << "\nC_Lt[" << k << "] = " << C_Lt[k];
        }
    }
}

void WavePlayer::setSeriesCoeffs_Unsafe(float *C_rt, unsigned int n_TermsRt, float *C_lt, unsigned int n_TermsLt)
{
    // std::cout << "WavePlayer::setSeriesCoeffs_Unsafe" << std::endl;
    C_Rt = C_rt;
    nTermsRt = n_TermsRt;
    C_Lt = C_lt;
    nTermsLt = n_TermsLt;

    // coefficients must sum to 1.0f
    // float sum = 0.0f;
    // if( C_Rt )
    // {
    //     for( unsigned int k = 0; k < nTermsRt; ++k )
    //     {
    //         if( C_Rt[k] > 0.0f ) sum += C_Rt[k];
    //         else sum -= C_Rt[k];
    //     }
    //     for( unsigned int k = 0; k < nTermsRt; ++k )
    //     {
    //         C_Rt[k] /= sum;
    //         // std::cout << "\nC_Rt[" << k << "] = " << C_Rt[k];
    //     }
    // }
    //
    // if( C_Lt )
    // {
    //     sum = 0.0f;
    //     for( unsigned int k = 0; k < nTermsLt; ++k )
    //     {
    //         if( C_Lt[k] > 0.0f ) sum += C_Lt[k];
    //         else sum -= C_Lt[k];
    //     }
    //     for( unsigned int k = 0; k < nTermsLt; ++k )
    //     {
    //         C_Lt[k] /= sum;
    //         // std::cout << "\nC_Lt[" << k << "] = " << C_Lt[k];
    //     }
    // }
}

void WavePlayer::update(float dt)
{
    if (!rightTrigFunc || !leftTrigFunc)
    {
        return;
    }
    tElapRt += dt;
    if (tElapRt > periodRt) tElapRt -= periodRt;
    tElapLt += dt;
    if (tElapLt > periodLt) tElapLt -= periodLt;

    float fr = 0.0f, fg = 0.0f, fb = 0.0f;// result
    float arg = 0.0f;

    for (unsigned int n = 0; n < numLts; ++n)
    {
        float yRt = 0.0f;
        arg = ((float) n / wvLenRt - tElapRt / periodRt) * 6.283f;
        if (C_Rt)
        {
            for (unsigned k = 0; k < nTermsRt; ++k)
                yRt += C_Rt[k] * rightTrigFunc((k + 1) * arg);
        }
        else
            yRt = rightTrigFunc(arg);

        float yLt = 0.0f;
        arg = ((float) n / wvLenLt + tElapLt / periodLt) * 6.283f;
        if (C_Lt)
        {
            for (unsigned k = 0; k < nTermsLt; ++k)
                yLt += C_Lt[k] * leftTrigFunc((k + 1) * arg);
        }
        else
            yLt = leftTrigFunc(arg);

        float y = AmpRt * yRt + AmpLt * yLt;
        //      std::cout << "\nwvPlay.update() yLt = " << yLt << " yRt = " << yRt << " y = " << y;

              // if( y < -1.0f || y > 1.0f )
              //     std::cout << "\nwvPlay.update() y = " << y;

        fr = 0.5f * ((y + 1.0f) * frHi - (y - 1.0f) * frLo);
        fg = 0.5f * ((y + 1.0f) * fgHi - (y - 1.0f) * fgLo);
        fb = 0.5f * ((y + 1.0f) * fbHi - (y - 1.0f) * fbLo);
        *(pLt0 + n) = Light(fr, fg, fb);
        
        //     unsigned int rd = ( pLt0 + n )->r, gn = ( pLt0 + n )->g, bu = ( pLt0 + n )->g;
        //     std::cout << "\nwvPlay.update() r = " << rd << " g = " << gn << " b = " << bu;
        //     std::cout << "\nwvPlay.update() fr = " << fr << " fg = " << fg << " fb = " << fb;
    }
}
