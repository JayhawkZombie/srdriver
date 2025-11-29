#ifndef PULSEPLAYER_H
#define PULSEPLAYER_H

#include<vector>
#include "Light.h"

class PulsePlayer
{
public:
    int W = 8, hfW = 4;// width of pulse
    float speed = 10.0f, tElap = 1.0f;// assign = 0 to trigger pulse
    bool doRepeat = true;
    float Trepeat = 1.0f;// time between pulses

    Light hiLt, loLt;// interpolate
    float frHi = 0.0f, fgHi = 0.0f, fbHi = 0.0f;// store once
    float frLo = 0.0f, fgLo = 0.0f, fbLo = 0.0f;// used in update

    unsigned int funcIdx = 0;
    float get_y(float u)const;// switches on funcIdx
    Light get_Lt(float y)const;// interpolate between member loLt and hiLt
    Light get_Lt(float y, unsigned int nLo)const;// interpolate  between existing color and hiLt

    int get_n0()const { return tElap * speed - W; }
    int get_nMid()const { return tElap * speed - hfW; }

    void update(float dt);// pulse travels left to right
    void updateLeft(float dt, bool drawFirst = true);// pulse travels right to left
    void update(std::vector<float> &yVec, float dt);// for graphing of pulse amplitude

    void init(Light &r_Lt0, unsigned int Rows, unsigned int Cols, Light HiLt, Light LoLt,
        int W_pulse, float Speed, float T_repeat, bool DoRepeat = true);

    PulsePlayer();
    ~PulsePlayer();

protected:
    Light *pLt0 = nullptr;
    unsigned int rows = 1, cols = 1;
    // dependent
    unsigned int numLts = 1;// = rows*cols

private:
};

#endif // PULSEPLAYER_H
