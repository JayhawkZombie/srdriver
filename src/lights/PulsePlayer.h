#ifndef PULSEPLAYER_H
#define PULSEPLAYER_H

#include<vector>
#include "Light.h"

struct PulsePlayerConfig {
    Light hiLt, loLt;
    float speed = 10.0f;
    float Trepeat = 1.0f;
    bool doRepeat = true;
};

class PulsePlayer
{
public:
    Light *pLt0 = nullptr;
    int numLts = 0;

    int hfW = 4;// width of pulse
    float speed = 10.0f, tElap = 1.0f;// assign = 0 to trigger pulse
    bool doRepeat = true;

    // the pulse color to blend in
    float fRd = 0.0f, fGn = 0.0f, fBu = 0.0f;// store once
    void setColor(uint8_t rd, uint8_t gn, uint8_t bu)
    {
        fRd = (float) rd; fGn = (float) gn; fBu = (float) bu;
    }
    void setColor(Light Lt)
    {
        setColor(Lt.r, Lt.g, Lt.b);
    }

    unsigned int funcIdx = 0;
    float get_y(float u)const;// switches on funcIdx
    Light get_Lt(float y, unsigned int nLo)const;// interpolate  between existing color and hiLt

    int get_n0()const { return tElap * speed - hfW; }
    int get_nMid()const { return tElap * speed; }

    void update(float dt);// pulse travels left to right
    void setPosition(int n) { tElap = n / speed; }// assign center position

    void init(Light &r_Lt0, int NumLts, Light HiLt, int W_pulse, float Speed, bool DoRepeat);
    void Start();

    PulsePlayer() {}
    ~PulsePlayer() {}

protected:
    void updateLeft(float dt);// for speed < 0
};

#endif // PULSEPLAYER_H
