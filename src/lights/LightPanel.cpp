#include "LightPanel.h"

/*
void LightPanel::update()const// write to target array
{
    // Light in source
    const Light* pSrcBase = pSrc0 + row0*srcCols + col0;

    for( int r = 0; r < rows; ++r )
    {
        const Light* pSrcRow = pSrcBase + r*srcCols;
        Light* pTgtRow = pTgt0 + r*cols;
        for( int c = 0; c < cols; ++c )
        {
          if( rotIdx == 0 )
            pTgtRow[c] = pSrcRow[c];
          else
            *( mapLightPosition( r, c ) ) = pSrcRow[c];
        }
    }

    if( type == 2 ) reverseOddRows( true );
}
*/

void LightPanel::updateSideways()const
{
    if (!swapTgtRCs) return;

    const Light *pSrcBase = pSrc0 + row0 * srcCols + col0;

    if (rotIdx == 0)
    {
        Light *pTgtBase = pTgt0 + rows * (cols - 1);// start of last row
        for (int r = 0; r < rows; ++r)
        {
            const Light *pSrcRow = pSrcBase + r * srcCols;
            for (int c = 0; c < cols; ++c)
            {
                pTgtBase[r - c * rows] = pSrcRow[c];
            }
        }
    }
    else if (rotIdx == 2)
    {
        Light *pTgtBase = pTgt0 + (rows - 1);// end of 1st row
        for (int r = 0; r < rows; ++r)
        {
            const Light *pSrcRow = pSrcBase + r * srcCols;
            for (int c = 0; c < cols; ++c)
            {
                pTgtBase[c * rows - r] = pSrcRow[c];
            }
        }
    }

    if (type == 2) reverseOddRowsSideways();
}

void LightPanel::update()const// write to target array
{
    if (swapTgtRCs)
    {
        updateSideways();
        return;
    }

    if (rotIdx == 0)// no rotation
    {
        const Light *pSrcBase = pSrc0 + row0 * srcCols + col0;
        for (int r = 0; r < rows; ++r)
        {
            const Light *pSrcRow = pSrcBase + r * srcCols;
            Light *pTgtRow = pTgt0 + r * cols;
            for (int c = 0; c < cols; ++c)
            {
                pTgtRow[c] = pSrcRow[c];
            }
        }

        if (type == 2) reverseOddRows(true);
        return;
    }

    // rotate
    switch (rotIdx)
    {
        case 1:// 90 degrees cw
            rotateCW();
            break;
        case -1:// 90 degrees ccw
            rotateCCW();
            break;

        case 2:// 180 degrees
        case -2:// 180 degrees
            rotate180();
            break;

        default: break;

    }

    if (type == 2) reverseOddRows(true);

    return;
}

void LightPanel::reverseOddRows(bool inTarget)const// for type = 2
{
    if (swapTgtRCs)
    {
        reverseOddRowsSideways();
        return;
    }

    Light *itBegin = inTarget ? pTgt0 : pSrc0;
    for (int r = 0; r < rows; r += 2)
    {
        Light *itLt = itBegin + r * cols;
        Light *itRt = itLt + cols - 1;
        while (itLt < itRt)
        {
            Light tempLt = *itLt;
            *itLt = *itRt;
            *itRt = tempLt;
            ++itLt;
            --itRt;
        }
    }
}

void LightPanel::reverseOddRowsSideways()const// for type = 2
{
    Light *itBegin = pTgt0;
    for (int c = 0; c < cols; c += 2)
    {
        Light *itLt = itBegin + c * rows;
        Light *itRt = itLt + rows - 1;
        while (itLt < itRt)
        {
            Light tempLt = *itLt;
            *itLt = *itRt;
            *itRt = tempLt;
            ++itLt;
            --itRt;
        }
    }
}

// entire panel
void LightPanel::rotateCW()const// rotate image 90 degrees clockwise
{
    if (rows != cols) return;
    const Light *pSrcBase = pSrc0 + row0 * srcCols + col0;// Light in source
    Light *pLtCorner = pTgt0 + cols - 1;// lower left corner of target

    for (int r = 0; r < rows; ++r)
    {
        const Light *pSrcRow = pSrcBase + r * srcCols;// source
        for (int c = 0; c < cols; ++c)
        {
            Light *pTgtRow = pLtCorner + c * cols;// target
            pTgtRow[-r] = pSrcRow[c];
        }
    }
}

void LightPanel::rotateCCW()const// 90 degrees counter clockwise
{
    if (rows != cols) return;
    const Light *pSrcBase = pSrc0 + row0 * srcCols + col0;// Light in source
    Light *pLtCorner = pTgt0 + cols * (rows - 1);// lower left corner of target

    for (int r = 0; r < rows; ++r)
    {
        const Light *pSrcRow = pSrcBase + r * srcCols;// source
        for (int c = 0; c < cols; ++c)
        {
            Light *pTgtRow = pLtCorner - c * cols;// target
            pTgtRow[r] = pSrcRow[c];
        }
    }
}

void LightPanel::rotate180()const// rotate image 180 degrees
{
    const Light *pSrcBase = pSrc0 + row0 * srcCols + col0;// Light in source
    Light *pLtCorner = pTgt0 + rows * cols - 1;// lower right corner of target

    for (int r = 0; r < rows; ++r)
    {
        const Light *pSrcRow = pSrcBase + r * srcCols;// source
        Light *pTgtRow = pLtCorner - r * cols;// target
        for (int c = 0; c < cols; ++c)
        {
            pTgtRow[-c] = pSrcRow[c];
        }
    }
}