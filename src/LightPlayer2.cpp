#include "LightPlayer2.h"

void LightPlayer2::init( Light& r_Lt0, int Rows, int Cols, const patternData& rPattData, unsigned int NumPatterns )
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

    // default is entire grid
    gridRows = rows;
    gridCols = cols;
    row0 = col0 = 0;
}

void LightPlayer2::update()// assign as desired
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

void LightPlayer2::updateOnOnly()// for drawing after another player
{
    for( unsigned int n = 0; n < numLts; ++n )
        if( getState(n) ) *( pLt0 + n ) = onLt;

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

void LightPlayer2::updateSub()
{
    Light* pBase = pLt0 + gridCols*row0 + col0;
 //   if( col0 < 0 ) std::cout << "\n top: col0 < 0";

    for( int r = 0; r < rows; ++r )
    {
        if( r + row0 < 0 ) continue;
        if( r + row0 >= gridRows ) break;

        Light* pRow = pBase + r*gridCols;
        for( int c = 0; c < cols; ++c )
        {
            if( c + col0 < 0 ) continue;
            if( c + col0 >= gridCols ) break;
            if( getState( r*cols + c ) ) *( pRow + c ) = onLt;
            else *( pRow + c ) = offLt;
        }
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

void LightPlayer2::updateSubOnOnly()// writes only to onLt
{
    Light* pBase = pLt0 + gridCols*row0 + col0;
    for( int r = 0; r < rows; ++r )
    {
        if( r + row0 < 0 ) continue;
        if( r + row0 >= gridRows ) break;

        Light* pRow = pBase + r*gridCols;
        for( int c = 0; c < cols; ++c )
        {
            if( c + col0 < 0 ) continue;
            if( c + col0 >= gridCols ) break;
            if( getState( r*cols + c ) )
                *( pRow + c ) = onLt;
        }
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

    if( funcIdx == 0 ) return 1;// pause pattern
    if( funcIdx >= 1 && funcIdx <= 5 ) return numLts;
    if( funcIdx == 6 ) return pattData[ patternIter ].param;// alternateBlink
    if( funcIdx == 7 ) return pattData[ patternIter ].param;// checkerBlink
    if( funcIdx == 10 || funcIdx == 11 ) return cols;// scrollCol
    if( funcIdx == 12 || funcIdx == 13 ) return rows;// scrollRow
    if( funcIdx == 14 || funcIdx == 15 ) return cols/2;// BoxIn, BoxOut
    if( funcIdx == 16 ) return rows + cols;// scrollDiagonal
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
        case 10 : return scrollColToRight( n );
        case 11 : return scrollColToLeft( n );
        case 12 : return scrollRowToBottom( n );
        case 13 : return scrollRowToTop( n );
        case 14 : return scrollBoxIn( n );
        case 15 : return scrollBoxOut( n );
        case 16 : return scrollDiagonal( n, param );

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
bool LightPlayer2::scrollColToRight( unsigned int n )const
{ return stepIter == n%cols; }

bool LightPlayer2::scrollColToLeft( unsigned int n )const
{ return stepIter == ( cols - 1 - n%cols ); }

bool LightPlayer2::scrollRowToBottom( unsigned int n )const
{ return stepIter == n/cols; }

bool LightPlayer2::scrollRowToTop( unsigned int n )const
{ return stepIter == rows - 1 - n/cols; }

bool LightPlayer2::scrollBoxIn( unsigned int n )const
{
    int Cmax = cols - 1 - stepIter;
    int Rmax = rows - 1 - stepIter;
    int r = n/cols, c = n%cols;

    if( ( r == (int)stepIter || r == Rmax ) && ( c >= (int)stepIter && c <= Cmax ) ) return true;
    if( ( c == (int)stepIter || c == Cmax ) && ( r >= (int)stepIter && r <= Rmax ) ) return true;
    return false;
}

bool LightPlayer2::scrollBoxOut( unsigned int n )const
{
    int Cmax = cols/2 + stepIter;
    int Cmin = cols - 1 - Cmax;
    int Rmax = rows/2 + stepIter;
    int Rmin = rows - 1 - Rmax;
    int r = n/cols, c = n%cols;

    if( ( r == Rmin || r == Rmax ) && ( c >= Cmin && c <= Cmax ) ) return true;
    if( ( c == Cmin || c == Cmax ) && ( r >= Rmin && r <= Rmax ) ) return true;
    return false;
}

// 0 = dn rt, 1 = up lt, 2 = dn lt, 3 = up rt
bool LightPlayer2::scrollDiagonal( unsigned int n, unsigned int Mode )const
{
    int r = n/cols, c = n%cols;
    if( Mode == 0 && (int)stepIter >= r ) return c == (int)stepIter - r;// dn rt
    if( Mode == 1 ) return c == cols - 1 - (int)stepIter + rows - 1 - r;// up lt
    if( Mode == 2 ) return c == cols - 1 - (int)stepIter + r;// dn lt
    if( Mode == 3 ) return c == (int)stepIter + r - rows - 1;// up rt

    return false;
}
