#include "PulsePlayer.h"

PulsePlayer::PulsePlayer()
{
    //ctor
}

PulsePlayer::~PulsePlayer()
{
    //dtor
}

void PulsePlayer::init(Light &r_Lt0, unsigned int Rows, unsigned int Cols, Light HiLt, Light LoLt,
    int W_pulse, float Speed, float T_repeat, bool DoRepeat)
{
    pLt0 = &r_Lt0;
    rows = Rows;
    cols = Cols;
    numLts = rows * cols;// cached
    hiLt = HiLt;
    loLt = LoLt;
    W = W_pulse;
    hfW = W / 2;// cached
    W = 2 * hfW;// W even only
    speed = Speed;
    Trepeat = T_repeat;
    doRepeat = DoRepeat;
    tElap = doRepeat ? 0.0f : (numLts + W) / speed + 1.0f;// off right end

    frHi = (float) hiLt.r; fgHi = (float) hiLt.g; fbHi = (float) hiLt.b;
    frLo = (float) loLt.r; fgLo = (float) loLt.g; fbLo = (float) loLt.b;
}

void PulsePlayer::update(float dt)
{
    if (tElap * speed < numLts + W)
    {
        tElap += dt;
        if (doRepeat && tElap * speed > numLts + W)
            tElap = 0.0f;

        int n0 = tElap * speed - W;
        if (n0 + W < 0 || n0 >= (int) numLts)// off left or right end
        {
            for (unsigned int n = 0; n < numLts; ++n)
                *(pLt0 + n) = loLt;// all loLt
        }
        else// pulse is in view
        {
            for (int n = 0; n < (int) numLts; ++n)
            {
                if (n < n0) *(pLt0 + n) = loLt;// before pulse
                else if (n >= n0 + W) *(pLt0 + n) = loLt;// after pulse
                else if (n < n0 + hfW)// left half: n >= n0 && n < n0 + hfW
                {
                    //      float u = (float)( n - n0 )/(float)hfW;// u: 0 to 1
                    float u = (float) (1 + n - n0) / (float) hfW;// u: 0 to 1
                    *(pLt0 + n) = get_Lt(get_y(u));
                }
                else// right half: n >= n0 + hfW && n < n0 + W
                {
                    float u = 1.0f - (float) (1 + n - n0 - hfW) / (float) hfW;// u: 1 to 0
                    *(pLt0 + n) = get_Lt(get_y(u));
                }
            }
        }// end draw pulse

    }// end if
    else
        for (unsigned int n = 0; n < numLts; ++n)
            *(pLt0 + n) = loLt;// all loLt

}

float PulsePlayer::get_y(float u)const// switches on funcIdx
{
    switch (funcIdx)
    {
        case 0: return u;// yp(0) = yp(1) = 1 a line
        case 1: return u * (2.0f - u);// yp(0) = 2, yp(1) = 0 quadratic
        case 2: return u * u * (3.0f - 2.0f * u);// yp(0) = yp(1) = 0 cubic
    }

    return 0.0f;
}

Light PulsePlayer::get_Lt(float y)const// interpolate
{
    float fr = (1.0f - y) * frLo + y * frHi;
    float fg = (1.0f - y) * fgLo + y * fgHi;
    float fb = (1.0f - y) * fbLo + y * fbHi;
    return Light(fr, fg, fb);
}

Light PulsePlayer::get_Lt(float y, unsigned int nLo)const// interpolate  between existing color and hiLt
{
    const Light &ExLt = *(pLt0 + nLo);
    float rLo = (float) ExLt.r, gLo = (float) ExLt.g, bLo = (float) ExLt.b;
    float fr = (1.0f - y) * rLo + y * frHi;
    float fg = (1.0f - y) * gLo + y * fgHi;
    float fb = (1.0f - y) * bLo + y * fbHi;
    return Light(fr, fg, fb);
}

void PulsePlayer::updateLeft(float dt, bool drawFirst)// pulse travels right to left
{
    if (tElap * speed < numLts + W)
    {
        tElap += dt;
        if (doRepeat && tElap * speed > numLts + W)
            tElap = 0.0f;

        //   int n0 = tElap*speed - W;
        int n0 = numLts - 1 - tElap * speed;

        if (n0 + W < 0 || n0 >= (int) numLts)// off left or right end
        {
            if (!drawFirst) return;

            for (unsigned int n = 0; n < numLts; ++n)
                *(pLt0 + n) = loLt;// all loLt
        }
        else// pulse is in view
        {
            for (int n = 0; n < (int) numLts; ++n)
            {
                if (n < n0)
                {
                    if (!drawFirst) continue;
                    *(pLt0 + n) = loLt;// before pulse
                }
                else if (n >= n0 + W)
                {
                    if (!drawFirst) continue;
                    *(pLt0 + n) = loLt;// after pulse
                }
                else if (n < n0 + hfW)// left half: n >= n0 && n < n0 + hfW
                {
                    float u = (float) (1 + n - n0) / (float) hfW;// u: 0 to 1
                    if (drawFirst) *(pLt0 + n) = get_Lt(get_y(u));
                    else *(pLt0 + n) = get_Lt(get_y(u), n);
                }
                else// right half: n >= n0 + hfW && n < n0 + W
                {
                    float u = 1.0f - (float) (1 + n - n0 - hfW) / (float) hfW;// u: 1 to 0
                    if (drawFirst) *(pLt0 + n) = get_Lt(get_y(u));
                    else *(pLt0 + n) = get_Lt(get_y(u), n);
                }
            }
        }// end draw pulse

    }// end if
    else if (drawFirst)
        for (unsigned int n = 0; n < numLts; ++n)
            *(pLt0 + n) = loLt;// all loLt
}

void PulsePlayer::update(std::vector<float> &yVec, float dt)// for graphing of pulse amplitude
{
    if (yVec.size() < numLts) return;

    if (tElap * speed < numLts + W)
    {
        tElap += dt;
        if (doRepeat && tElap * speed > numLts + W)
            tElap = 0.0f;

        int n0 = tElap * speed - W;
        if (n0 + W < 0 || n0 >= (int) numLts)// off left or right end
        {
            for (unsigned int n = 0; n < numLts; ++n)
            {
                yVec[n] = 0.0f;
                *(pLt0 + n) = loLt;// all loLt
            }
        }
        else// pulse is in view
        {
            for (int n = 0; n < (int) numLts; ++n)
            {
                if (n < n0)
                {
                    yVec[n] = 0.0f;
                    *(pLt0 + n) = loLt;// before pulse
                }
                else if (n >= n0 + W)
                {
                    yVec[n] = 0.0f;
                    *(pLt0 + n) = loLt;// after pulse
                }
                else if (n < n0 + hfW)// left half: n >= n0 && n < n0 + hfW
                {
                    //      float u = (float)( n - n0 )/(float)hfW;// u: 0 to 1
                    float u = (float) (1 + n - n0) / (float) hfW;// u: 0 to 1
                    float y = get_y(u);
                    yVec[n] = y;
                    *(pLt0 + n) = get_Lt(y);
                }
                else// right half: n >= n0 + hfW && n < n0 + W
                {
                    float u = 1.0f - (float) (1 + n - n0 - hfW) / (float) hfW;// u: 1 to 0
                    float y = get_y(u);
                    yVec[n] = y;
                    *(pLt0 + n) = get_Lt(y);
                }
            }
        }// end draw pulse

    }// end if
    else
        for (unsigned int n = 0; n < numLts; ++n)
            *(pLt0 + n) = loLt;// all loLt
}
