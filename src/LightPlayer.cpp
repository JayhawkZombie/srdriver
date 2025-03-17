#include "LightPlayer.h"

LightPlayer::LightPlayer()
{
    //ctor
}

LightPlayer::~LightPlayer()
{
    //dtor
}

void LightPlayer::init( Light& r_Lt0, unsigned int Rows, unsigned int Cols )
{
    pLt0 = &r_Lt0;
    rows = Rows;
    cols = Cols;
    numLts = rows*cols;

    numStates = 5;// 5 active
//    iterMax = numStates*numLts*numLts + numBlinks*blinkPause*numLts + numLts*endPause;
    iterMax = numLts*( numStates*numLts + numBlinks*blinkPause + endPause );
    iter = 0;
}

void LightPlayer::update( const Light& onLt, const Light& offLt )
{
    for( unsigned int n = 0; n < numLts; ++n )
    {
        if( getState() ) *( pLt0 + n ) = onLt;
        else *( pLt0 + n ) = offLt;

        ++iter;
    }

    if( iter >= iterMax ) iter = 0;// restart pattern
}

bool LightPlayer::getState()
{
//    unsigned int step = iter/numLts;
    unsigned int step = iter/numLts;
    unsigned int n = iter%numLts;

 //   return k == step;


    // across from left
//    if( step < numLts ) return k == step;
    if( step < numLts ) return ( n >= step && n < step + k1 );
    step -= numLts;

    // across from right
 //   if( step < numLts ) return k == numLts - 1 - step;
    if( step < numLts ) return ( n <= numLts - 1 - step ) && ( n + k2 > numLts - 1 - step );
    step -= numLts;

    // fill from left
    if( step < numLts ) return n <= step;
    step -= numLts;

    // criss cross k1 each way
    if( step < numLts )
    {
    //    bool A = (k == step);
    //    bool B = (k == numLts - 1 - step);
        bool A = ( n >= step && n < step + k1 );
        bool B = ( n <= numLts - 1 - step);
        B = B && ( n + k1 > numLts - 1 - step );

        return A || B;
    }
    step -= numLts;

    // fill from right
    if( step < numLts ) return n >= numLts - 1 - step;
    step -= numLts;

    // alternate blink numBlinks times
    if( step < numBlinks*blinkPause ) return ( n + step/blinkPause )%2;
    step -= numBlinks*blinkPause;

    // leftover = delay before restart
    return false;

}
