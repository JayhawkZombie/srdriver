#ifndef RINGPLAYER_H
#define RINGPLAYER_H

#include<cmath>
#include "../Light.h"
// #include "FileParser.h"

struct RPdata
{
    Light hiLt, loLt;
    float fRowC = 0.0f;
    float fColC = 0.0f;
    float ringSpeed = 100.0f;// center radius = ringSpeed*tElap
    float ringWidth = 2.0f;// Light spaces
    float fadeRadius = 50.0f;// no fade for high value. Animation ends when no Light is written to.
    float fadeWidth = 4.0f;
    float Amp = 1.0f;// limit blending of hiLt and loLt
    // bool init( FileParser& FP );
};

class RingPlayer
{
    public:
    Light* pLt0 = nullptr;// to LightArr
    int rows = 1, cols = 1;// dimensions of LightArr array
    Light hiLt, loLt;
    float fRowC = 0.0f, fColC = 0.0f;
    float tElap = 0.0f;
    float ringSpeed = 100.0f;// center radius = ringSpeed*tElap
    float ringWidth = 2.0f;// Light spaces
    float fadeRadius = 50.0f;// no fade for high value. Animation ends when no Light is written to.
    float fadeWidth = 4.0f;
    float Amp = 1.0f;// limit blending of hiLt and loLt

    void initToGrid( Light* p_Lt0, int gridRows, int gridCols )
    { pLt0 = p_Lt0; rows = gridRows; cols = gridCols; }

    void setRingCenter( float rowC, float colC )
    { fRowC = rowC; fColC = colC; }

    void setRingProps( float Speed, float RingWidth, float FadeRadius, float FadeWidth )
    {
        ringSpeed = Speed;// center radius = ringSpeed*tElap
        ringWidth = RingWidth;// Light spaces
        fadeRadius = FadeRadius;// no fade for high value
        fadeWidth = FadeWidth;// fade rate
    }

    void setup( const RPdata& RPD )
    {
        setRingProps( RPD.ringSpeed, RPD.ringWidth, RPD.fadeRadius, RPD.fadeWidth );
        setRingCenter( RPD.fColC, RPD.fRowC );
        hiLt = RPD.hiLt;
        loLt = RPD.loLt;
        Amp = RPD.Amp;
    }

    bool isPlaying = false;
    bool isVisible = false;// allows off grid wave to grow
    bool onePulse = true;
    bool isRadiating = false;// wave
    int direction = 1;
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
        isVisible = false;
        if( onePulse )
        {
            tElap = -0.8f*ringWidth/ringSpeed;
            isRadiating = false;
        }
        else// radiating wave
        {
            tElap = 0.0f;
            isRadiating = true;
        }
    }

    bool update( float dt );// true if animating
    // for each process
    void updatePulse( float dt );
    void updateWave( float dt );

    RingPlayer(){}
    ~RingPlayer(){}

    // static methods for use with arrays. Visit each Light just once to update for all
    static void updatePulseAll( RingPlayer* pRP, int numRP, float dt, float* FloatAll, bool* LtAssAll );

    protected:

    private:
};

#endif // RINGPLAYER_H
