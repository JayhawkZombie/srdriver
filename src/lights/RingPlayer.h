#ifndef RINGPLAYER_H
#define RINGPLAYER_H

#include<cmath>
#include "Light.h"

class RingPlayer
{
public:
    Light *pLt0 = nullptr;// to LightArr
    int rows = 1, cols = 1;// dimensions of LightArr array
    Light hiLt, loLt;
    float fRowC = 0.0f, fColC = 0.0f;
    float tElap = 0.0f;
    float ringSpeed = 100.0f;// center radius = ringSpeed*tElap
    float ringWidth = 2.0f;// Light spaces
    float fadeRadius = 50.0f;// no fade for high value. Animation ends when no Light is written to.
    float fadeWidth = 4.0f;
    float Amp = 1.0f;// limit blending of hiLt and loLt

    void initToGrid(Light *p_Lt0, int gridRows, int gridCols)
    {
        pLt0 = p_Lt0; rows = gridRows; cols = gridCols;
    }

    void setRingCenter(float rowC, float colC)
    {
        fRowC = rowC; fColC = colC;
    }

    void setRingProps(float Speed, float RingWidth, float FadeRadius, float FadeWidth)
    {
        ringSpeed = Speed;// center radius = ringSpeed*tElap
        ringWidth = RingWidth;// Light spaces
        fadeRadius = FadeRadius;// no fade for high value
        fadeWidth = FadeWidth;// fade rate
    }

    int direction = 1;// 1 = out, -1 = in
    bool isPlaying = false;
    bool onePulse = true;
    bool isRadiating = false;// wave
    float stopTime = 0.0f;// no write for R < stopTime*ringSpeed
    void StopWave()
    {
        stopTime = 0.0f;
        isRadiating = false;
    }

    void Start()
    {
        stopTime = 0.0f;
        isPlaying = true;
        if (onePulse)
        {
            tElap = -0.8f * ringWidth / ringSpeed;
            isRadiating = false;
        }
        else// radiating wave
        {
            tElap = 0.0f;
            isRadiating = true;
        }
    }

    bool update(float dt);// true if animating
    // for each process
    void updatePulse(float dt);
    void updateWave(float dt);

    RingPlayer() {}
    ~RingPlayer() {}

protected:

private:
};

#endif // RINGPLAYER_H
