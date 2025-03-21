#include "LightPlayer2.h"

void LightPlayer2::init( Light& r_Lt0, unsigned int Rows, unsigned int Cols, const patternData& rPattData, unsigned int NumPatterns )
{
    pLt0 = &r_Lt0;
    rows = Rows;
    cols = Cols;
    numLts = rows*cols;

    stepTimer = 0;
    stepIter = 0;
    patternIter = 0;

    pattData = &rPattData;
    numPatterns = NumPatterns;
}

void LightPlayer2::setStateData( uint8_t* p_StateData, unsigned int DataSz )
{
    pStateData = p_StateData;
    stateDataSz = DataSz;
    BA.init( p_StateData[0], DataSz );
}

void LightPlayer2::update( const Light& onLt, const Light& offLt )// assign as desired
{
    for( unsigned int n = 0; n < numLts; ++n )
    {
        if( getState(n) ) *( pLt0 + n ) = onLt;
        else *( pLt0 + n ) = offLt;
    }

    if( ++stepTimer >= pattData[ patternIter ].stepPause )
    {
        stepTimer = 0;// to next step
        if( ++stepIter >= getPattLength() )
        {
            stepIter = 0;// to next pattern
            if( ++patternIter >= numPatterns )
                patternIter = 0;// reset cycle
        }
    }
}

unsigned int LightPlayer2::getPattLength()const
{
    const unsigned int funcIdx = pattData[ patternIter ].funcIndex;

    if( funcIdx == 0 ) return 1;
    if( funcIdx >= 1 && funcIdx <= 5 ) return numLts;
    if( funcIdx == 6 ) return pattData[ patternIter ].param;// alternateBlink
    if( funcIdx == 7 ) return pattData[ patternIter ].param;// checkerBlink
    if( funcIdx == 10 || funcIdx == 11 ) return cols;// scrollCol
    if( funcIdx == 12 || funcIdx == 13 ) return rows;// scrollRow
    if( funcIdx == 14 || funcIdx == 15 ) return cols/2;// BoxIn, BoxOut
    if( funcIdx == 16 ) return rows + cols;// scrollDiagonal
 //   if( funcIdx == 100 ) return stateDataSz/numLts;// from pStateData[]
    if( funcIdx == 100 ) return BA.bitSize()/numLts;// from pStateData[]
    return 1;
}

bool LightPlayer2::getState( unsigned int n )const
{
    unsigned int funcIdx = pattData[ patternIter ].funcIndex;
    unsigned int param = pattData[ patternIter ].param;

    switch( funcIdx )
    {
        case 0 : return false;// a "pause" between patterns
        case 1 : return scrollToRight( n, param );
        case 2 : return scrollToLeft( n, param );
        case 3 : return fillFromRight( n );
        case 4 : return fillFromLeft( n );
        case 5 : return crissCross( n, param );
        case 6 : return alternateBlink( n );
        case 7 : return checkerBlink( n );
        // 2d patterns
        case 10 : return scrollColToRight( n, param );
        case 11 : return scrollColToLeft( n, param );
        case 12 : return scrollRowToBottom( n, param );
        case 13 : return scrollRowToTop( n, param );
        case 14 : return scrollBoxIn( n );
        case 15 : return scrollBoxOut( n );
        case 16 : return scrollDiagonal( n );

        case 100 :
       //     if( pStateData ) return pStateData[ stepIter*numLts + n ];
            if( pStateData ) return BA.getBit( stepIter*numLts + n );
            return false;

        default: return false;// offLight
    }

    return false;// offLight
}

bool LightPlayer2::scrollToRight( unsigned int n, unsigned int numInGroup )const// returns state assignment
{ return ( n >= stepIter && n < stepIter + numInGroup ); }

bool LightPlayer2::scrollToLeft( unsigned int n, unsigned int numInGroup )const// returns state assignment
{ return ( n <= numLts - 1 - stepIter ) && ( n + numInGroup > numLts - 1 - stepIter ); }

bool LightPlayer2::fillFromRight( unsigned int n )const
{ return ( n >= numLts - 1 - stepIter ); }

bool LightPlayer2::fillFromLeft( unsigned int n )const
{ return ( n <= stepIter ); }

bool LightPlayer2::crissCross( unsigned int n, unsigned int numInGroup )const
{
    bool A = ( n >= stepIter && n < stepIter + numInGroup );
    bool B = ( n <= numLts - 1 - stepIter );
    B = B && ( n + numInGroup > numLts - 1 - stepIter );
    return A || B;
}

bool LightPlayer2::alternateBlink( unsigned int n )const
{ return ( n + stepIter )%2; }

bool LightPlayer2::checkerBlink( unsigned int n )const
{ return ( n + n/cols + stepIter )%2; }

// patterns for 2d
bool LightPlayer2::scrollColToRight( unsigned int n, unsigned int numInGroup )const
{ return stepIter == n%cols; }

bool LightPlayer2::scrollColToLeft( unsigned int n, unsigned int numInGroup )const
{ return stepIter == ( cols - 1 - n%cols ); }

bool LightPlayer2::scrollRowToBottom( unsigned int n, unsigned int numInGroup )const
{ return stepIter == n/cols; }

bool LightPlayer2::scrollRowToTop( unsigned int n, unsigned int numInGroup )const
{ return stepIter == rows - 1 - n/cols; }

bool LightPlayer2::scrollBoxIn( unsigned int n )const
{
    unsigned int SI = stepIter, SIr = cols - 1 - stepIter;
    unsigned int ndc = n/cols, npc = n%cols;
    bool A = ( ndc >= SI ) && ( ndc <= SIr );
    bool B = ( npc >= SI ) && ( npc <= SIr );
    bool LC = ( npc == SI ) && A;
    bool RC = ( npc == SIr ) && A;
    bool TR = ( ndc == SI ) && B;
    bool BR = ( ndc == SIr ) && B;
    return LC || RC || TR || BR;
}

bool LightPlayer2::scrollBoxOut( unsigned int n )const
{
    unsigned int SI = cols/2 - 1 - stepIter;
    unsigned int SIr = cols - 1 - SI;
    unsigned int ndc = n/cols, npc = n%cols;
    bool A = ( ndc >= SI ) && ( ndc <= SIr );
    bool B = ( npc >= SI ) && ( npc <= SIr );
    bool LC = ( npc == SI ) && A;
    bool RC = ( npc == SIr ) && A;
    bool TR = ( ndc == SI ) && B;
    bool BR = ( ndc == SIr ) && B;
    return LC || RC || TR || BR;
}

bool LightPlayer2::scrollDiagonal( unsigned int n )const
{
    if( stepIter >= n/cols ) return n%cols == stepIter - n/cols;
    return false;
}
