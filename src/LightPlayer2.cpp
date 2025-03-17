#include "LightPlayer2.h"

LightPlayer2::LightPlayer2()
{
    //ctor
}

LightPlayer2::~LightPlayer2()
{
    //dtor
}

void LightPlayer2::init( Light& r_Lt0, unsigned int Rows, unsigned int Cols )
{
    pLt0 = &r_Lt0;
    rows = Rows;
    cols = Cols;
    numLts = rows*cols;

    stepTimer = 0;
    stepIter = 0;
    patternIter = 0;
}

void LightPlayer2::setArrays( unsigned int NumPatterns, const unsigned int* StepPause,
    const unsigned int* PatternLength, const unsigned int* FuncIndex, const unsigned int* FuncParam )
{
    numPatterns = NumPatterns;
    stepPause = StepPause;
    patternLength = PatternLength;
    funcIndex = FuncIndex;
    funcParam = FuncParam;
}

void LightPlayer2::update( const Light& onLt, const Light& offLt )// assign as desired
{
    for( unsigned int n = 0; n < numLts; ++n )
    {
        if( getState(n) ) *( pLt0 + n ) = onLt;
        else *( pLt0 + n ) = offLt;
    }

    if( ++stepTimer >= stepPause[ patternIter ] )
    {
        stepTimer = 0;// to next step
        if( ++stepIter >= patternLength[ patternIter ] )
        {
            stepIter = 0;// to next pattern
            if( ++patternIter >= numPatterns )
                patternIter = 0;// reset cycle
        }
    }
}

bool LightPlayer2::getState( unsigned int n )const
{
    unsigned int funcIdx = funcIndex[ patternIter ];
    unsigned int param = funcParam[ patternIter ];

    switch( funcIdx )
    {
        case 0 : return false;// a "pause" between patterns
        case 1 : return scrollToRight( n, param );
        case 2 : return scrollToLeft( n, param );
        case 3 : return fillFromRight( n );
        case 4 : return fillFromLeft( n );
        case 5 : return crissCross( n, param );
        case 6 : return alternateBlink( n );

        default: return false;// offLight
    }

    return false;// offLight
}

bool LightPlayer2::scrollToRight( unsigned int n, unsigned int numInGroup )const// returns state assignment
{
    return ( n >= stepIter && n < stepIter + numInGroup );
}

bool LightPlayer2::scrollToLeft( unsigned int n, unsigned int numInGroup )const// returns state assignment
{
    return ( n <= numLts - 1 - stepIter ) && ( n + numInGroup > numLts - 1 - stepIter );
}

bool LightPlayer2::fillFromRight( unsigned int n )const
{
    return ( n >= numLts - 1 - stepIter );
}

bool LightPlayer2::fillFromLeft( unsigned int n )const
{
    return ( n <= stepIter );
}

bool LightPlayer2::crissCross( unsigned int n, unsigned int numInGroup )const
{
    bool A = ( n >= stepIter && n < stepIter + numInGroup );
    bool B = ( n <= numLts - 1 - stepIter );
    B = B && ( n + numInGroup > numLts - 1 - stepIter );
    return A || B;
}

bool LightPlayer2::alternateBlink( unsigned int n )const
{
    return ( n + stepIter )%2;
}
