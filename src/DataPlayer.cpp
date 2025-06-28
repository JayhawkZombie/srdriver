#include "DataPlayer.h"

void DataPlayer::init( Light& r_Lt0, int Rows, int Cols, uint8_t& r_StateData, unsigned int DataSz, uint8_t NumColors )
{
    pLt0 = &r_Lt0;
    rows = Rows;
    cols = Cols;
    numLts = rows*cols;

    stepTimer = 0;
    stepIter = 0;

    // default is entire grid
    gridRows = rows;
    gridCols = cols;
    row0 = col0 = 0;
    drawMode = 1;// is grid

    pStateData = &r_StateData;
    stateDataSz = DataSz;
    numColors = NumColors;
    BA.init( r_StateData, DataSz );


    if( numColors == 2 )
        numSteps = (8*DataSz)/numLts;
    else if( numColors == 4 )
        numSteps = (4*DataSz)/numLts;
    else if( numColors == 16 )
        numSteps = (2*DataSz)/numLts;
}

void DataPlayer::setGridBounds( int Row0, int Col0, int GridRows, int GridCols )
{
    row0 = Row0;
    col0 = Col0;
    gridRows = GridRows;
    gridCols = GridCols;

    if( rows == gridRows && cols == gridCols && row0 == 0 && col0 == 0 )
        drawMode = 1;// is grid
    else if( ( row0 >= 0 && row0 + rows <= gridRows ) && ( col0 >= 0 && col0 + cols <= gridCols ) )
        drawMode = 2;// is all in grid
    else
        drawMode = 3;// is partly in grid
}

void DataPlayer::updateIsGrid()// 1
{
    for( unsigned int n = 0; n < numLts; ++n )
    {
        if( fadeAlong )
            *( pLt0 + n ) = updateFade(n);
        else// regular draw
        {
            Light LtNow = getState(n);
            if( drawOff ) *( pLt0 + n ) = LtNow;// draw all colors
            else if( LtNow != Lt[0] ) *( pLt0 + n ) = LtNow;// draw all but Lt[0]
        }
    }

    if( !isPlaying ) return;

    if( ++stepTimer >= stepPause )
    {
        stepTimer = 0;// to next step
        if( ++stepIter >= numSteps )
            stepIter = 0;// start over
    }
}

void DataPlayer::updateAllIn()// 2
{
    Light* pBase = pLt0 + gridCols*row0 + col0;

    for( int r = 0; r < rows; ++r )
    {
        Light* pRow = pBase + r*gridCols;
        for( int c = 0; c < cols; ++c )
        {
            if( fadeAlong ) *( pRow + c ) = updateFade( r*cols + c );
            else// regular draw
            {
                Light LtNow = getState( r*cols + c );
                if( drawOff ) *( pRow + c ) = LtNow;// draw all colors
                else if( LtNow != Lt[0] ) *( pRow + c ) = LtNow;// draw all but Lt[0]
            }
        }
    }

    if( ++stepTimer >= stepPause )
    {
        stepTimer = 0;// to next step
        if( ++stepIter >= numSteps )
            stepIter = 0;// start over
    }
}

void DataPlayer::flipX_AllIn()
{
    Light* pBase = pLt0 + gridCols*row0 + col0;
    for( int r = 0; r < rows; ++r )
    {
        Light* pRow = pBase + r*gridCols;
        for( int c = 0; c < cols; ++c )
        {
            if( fadeAlong ) *( pRow + c ) = updateFade( r*cols + cols - 1 - c );
            else// regular draw
            {
                Light LtNow = getState( r*cols + cols - 1 - c );
                if( drawOff ) *( pRow + c ) = LtNow;// draw all colors
                else if( LtNow != Lt[0] ) *( pRow + c ) = LtNow;// draw all but Lt[0]
            }
        }
    }

    if( ++stepTimer >= stepPause )
    {
        stepTimer = 0;
        if( ++stepIter >= numSteps )
            stepIter = 0;
    }
}

void DataPlayer::flipY_AllIn()
{
    Light* pBase = pLt0 + gridCols*row0 + col0;
    for( int r = 0; r < rows; ++r )
    {
        Light* pRow = pBase + r*gridCols;
        for( int c = 0; c < cols; ++c )
        {
            if( fadeAlong ) *( pRow + c ) = updateFade( ( rows - 1 - r )*cols + c );
            else// regular draw
            {
                Light LtNow = getState( ( rows - 1 - r )*cols + c );
                if( drawOff ) *( pRow + c ) = LtNow;// draw all colors
                else if( LtNow != Lt[0] ) *( pRow + c ) = LtNow;// draw all but Lt[0]
            }
        }
    }

    if( ++stepTimer >= stepPause )
    {
        stepTimer = 0;
        if( ++stepIter >= numSteps )
            stepIter = 0;
    }
}

void DataPlayer::updatePartlyIn()// 3
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

            if( fadeAlong ) *( pRow + c ) = updateFade( r*cols + c );
            else// regular draw
            {
                Light LtNow = getState( r*cols + c );
                if( drawOff ) *( pRow + c ) = LtNow;// draw all colors
                else if( LtNow != Lt[0] ) *( pRow + c ) = LtNow;// draw all but Lt[0]
            }
        }
    }

    if( ++stepTimer >= stepPause )
    {
        stepTimer = 0;// to next step
        if( ++stepIter >= numSteps )
            stepIter = 0;// start over
    }
}

void DataPlayer::update()
{
    if( flipX )
    {
        if( drawMode == 1 || drawMode == 2 ) flipX_AllIn();
        else updatePartlyIn();// no flip is done here
    }
    else if( flipY )
    {
        if( drawMode == 1 || drawMode == 2 ) flipY_AllIn();
        else updatePartlyIn();// no flip is done here
    }
    else
    {
        if( drawMode == 1 ) updateIsGrid();
        else if( drawMode == 2 ) updateAllIn();
        else updatePartlyIn();
    }
}

Light DataPlayer::updateFade( unsigned int n )const
{
    Light LtNow = Lt[0], LtNext = Lt[0];
    unsigned int iterNext = ( stepIter + 1 )%numSteps;


    if( numColors == 2 )// 2 color
    {
        LtNow = BA.getBit( stepIter*numLts + n ) ? Lt[1] : Lt[0];
        LtNext = BA.getBit( iterNext*numLts + n ) ? Lt[1] : Lt[0];
    }
    else if( numColors == 4 )
    {
        LtNow = Lt[ BA.getDblBit( stepIter*numLts + n ) ];
        LtNext = Lt[ BA.getDblBit( iterNext*numLts + n ) ];
    }
    else if( numColors == 16 )
    {
        LtNow = Lt[ BA.getQuadBit( stepIter*numLts + n ) ];
        LtNext = Lt[ BA.getQuadBit( iterNext*numLts + n ) ];
    }

    if( LtNow == LtNext ) return LtNow;

    float u = (float)stepTimer/(float)stepPause;
    u = u*u*( 3.0f - 2.0f*u );

    float fr = u*LtNext.r + ( 1.0f - u )*LtNow.r;
    float fg = u*LtNext.g + ( 1.0f - u )*LtNow.g;
    float fb = u*LtNext.b + ( 1.0f - u )*LtNow.b;

    return Light( fr, fg, fb );
}

Light DataPlayer::getState( unsigned int n )const
{
    if( numColors == 2 )
    {
        if( BA.getBit( stepIter*numLts + n ) ) return Lt[1];
        return Lt[0];
    }

    if( numColors == 4 )
        return Lt[ BA.getDblBit( stepIter*numLts + n ) ];

    if( numColors == 16 )
        return Lt[ BA.getQuadBit( stepIter*numLts + n ) ];

    return Lt[0];
}

void DataPlayer::prevImage()
{
    stepTimer = 0;
    if( stepIter > 0 ) --stepIter;
    else stepIter = numSteps - 1;
}

void DataPlayer::nextImage()
{
    stepTimer = 0;
    if( ++stepIter >= numSteps )
        stepIter = 0;
}

void DataPlayer::showImage( unsigned int n )// 0 to numSteps - 1
{
    stepTimer = 0;
    stepIter = n%numSteps;// keep in range
}

// on grid only
void DataPlayer::showColors()const
{
    for( unsigned int n = 0; n < numLts; ++n )
    {
        if( n < numColors ) *( pLt0 + n ) = Lt[n];
        else *( pLt0 + n ) = Lt[0];
    }
}
