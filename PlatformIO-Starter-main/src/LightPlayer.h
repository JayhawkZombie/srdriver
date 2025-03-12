#ifndef LIGHTPLAYER_H
#define LIGHTPLAYER_H

#include "Light.h"

class LightPlayer
{
    public:

    unsigned int k1 = 3, k2 = 5;// number across in 1st 2 patterns
    unsigned int blinkPause = 5;
    unsigned int numBlinks = 10;
    unsigned int endPause = 30;

    void init( Light& r_Lt0, unsigned int Rows, unsigned int Cols );
    void update( const Light& onLt, const Light& offLt );// assign as desired
    bool getState();

    // getters
    unsigned int getIterMax()const{ return iterMax; }


    LightPlayer( Light& r_Lt0, unsigned int Rows, unsigned int Cols )
    { init( r_Lt0, Rows, Cols ); }

    LightPlayer();
    ~LightPlayer();

    protected:

    Light* pLt0 = nullptr;
    unsigned int rows = 1, cols = 1;
    unsigned int numStates = 1;// length of pattern
    unsigned int iterMax = 0;// numStates*rows*cols
    unsigned int iter = 0;// 0 to iterMax
    // dependent
    unsigned int numLts = 1;

    private:
};

#endif // LIGHTPLAYER_H
