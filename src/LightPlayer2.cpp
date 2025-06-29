#include "LightPlayer2.h"

template<typename T>
T getUpperBitsValue(const T &value, int N)
{
    return (value >> N);
}

template<typename T>
T getLowerBitsValue(const T &value, int N)
{
    int mask = ~(~0 << N);
    return ((value << N) >> N) & mask;
}

void LightPlayer2::init(Light &r_Lt0,
    int Rows,
    int Cols,
    const patternData &rPattData,
    unsigned int NumPatterns)
{
    pLt0 = &r_Lt0;
    rows = Rows;
    cols = Cols;
    numLts = rows * cols;

    stepTimer = 0;
    stepIter = 0;
    patternIter = 0;

    pattData = &rPattData;
    numPatterns = NumPatterns;

    // default is entire grid
    gridRows = rows;
    gridCols = cols;
    row0 = col0 = 0;
    drawMode = 1;
}

void LightPlayer2::bindToGrid(Light &r_Lt0, int GridRows, int GridCols)
{
    pLt0 = &r_Lt0;
    setGridBounds(row0, col0, GridRows, GridCols);
}

void LightPlayer2::setDrawMode()
{
    if (rows == gridRows && cols == gridCols && row0 == 0 && col0 == 0)
        drawMode = 1;// is grid
    else if ((row0 >= 0 && row0 + rows <= gridRows) && (col0 >= 0 && col0 + cols <= gridCols))
        drawMode = 2;// is all in grid
    else
        drawMode = 3;// is partly in grid
}

void LightPlayer2::firePattern(unsigned int pattIdx)
{
    if (pattIdx >= numPatterns) return;
    patternIter = pattIdx;
    stepIter = stepTimer = 0;
}

void LightPlayer2::setToPlaySinglePattern(bool playSingle)
{
    playSinglePattern = playSingle;
    if (playSingle)
    {
        patternIter = 0;
        stepIter = getPattLength();// so update() returns
    }
    else
    {
        stepIter = stepTimer = 0;
    }
}

void LightPlayer2::takeStep()
{
    if (++stepTimer >= pattData[patternIter].stepPause)
    {
        stepTimer = 0;// to next step
        if (++stepIter >= getPattLength())
        {
            if (playSinglePattern) return;// reset stepIter to replay pattern
            stepIter = 0;// to next pattern
            if (++patternIter >= numPatterns && doRepeatSeq)
                patternIter = 0;// reset cycle
        }
    }
}

void LightPlayer2::update()// assign as required
{
    if (patternIter >= numPatterns) return;
    if (playSinglePattern && stepIter >= getPattLength()) return;

    if (drawMode == 1)
    {
        if (drawOffLt) updateIsGrid();
        else updateIsGridOnOnly();
    }
    else if (drawMode == 2 || drawMode == 3)
    {
        if (drawOffLt) updateSub();
        else updateSubOnOnly();
    }
    else updateSub();// default

    takeStep();
}

void LightPlayer2::updateIsGrid()// assign as desired
{
    for (unsigned int n = 0; n < numLts; ++n)
    {
        if (getState(n)) *(pLt0 + n) = onLt;
        else *(pLt0 + n) = offLt;
    }

    //   takeStep();
}

void LightPlayer2::updateIsGridOnOnly()// for drawing after another player
{
    for (unsigned int n = 0; n < numLts; ++n)
        if (getState(n)) *(pLt0 + n) = onLt;

    //   takeStep();
}

void LightPlayer2::updateSub()
{
    Light *pBase = pLt0 + gridCols * row0 + col0;
    //   if( col0 < 0 ) std::cout << "\n top: col0 < 0";

    for (int r = 0; r < rows; ++r)
    {
        if (r + row0 < 0) continue;
        if (r + row0 >= gridRows) break;

        Light *pRow = pBase + r * gridCols;
        for (int c = 0; c < cols; ++c)
        {
            if (c + col0 < 0) continue;
            if (c + col0 >= gridCols) break;
            if (getState(r * cols + c)) *(pRow + c) = onLt;
            else *(pRow + c) = offLt;
        }
    }

    //   takeStep();
}

// void LightPlayer2::update() // assign as desired
// {
//     for (unsigned int n = 0; n < numLts; ++n)
//     {
//         if (getState(n)) *(pLt0 + n) = onLt;
//         else *(pLt0 + n) = offLt;
//     }

//     if (++stepTimer >= pattData[patternIter].stepPause)
//     {
//         stepTimer = 0; // to next step
//         if (++stepIter >= getPattLength())
//         {
//             stepIter = 0; // to next pattern
//             if (++patternIter >= numPatterns)
//                 patternIter = 0; // reset cycle
//         }
//     }
// }

// void LightPlayer2::updateOnOnly() // for drawing after another player
// {
//     for (unsigned int n = 0; n < numLts; ++n)
//         if (getState(n)) *(pLt0 + n) = onLt;

//     if (++stepTimer >= pattData[patternIter].stepPause)
//     {
//         stepTimer = 0; // to next step
//         if (++stepIter >= getPattLength())
//         {
//             stepIter = 0; // to next pattern
//             if (++patternIter >= numPatterns)
//                 patternIter = 0; // reset cycle
//         }
//     }
// }

// void LightPlayer2::updateSub()
// {
//     Light *pBase = pLt0 + gridCols * row0 + col0;
//     //   if( col0 < 0 ) std::cout << "\n top: col0 < 0";

//     for (int r = 0; r < rows; ++r)
//     {
//         if (r + row0 < 0) continue;
//         if (r + row0 >= gridRows) break;

//         Light *pRow = pBase + r * gridCols;
//         for (int c = 0; c < cols; ++c)
//         {
//             if (c + col0 < 0) continue;
//             if (c + col0 >= gridCols) break;
//             if (getState(r * cols + c)) *(pRow + c) = onLt;
//             else *(pRow + c) = offLt;
//         }
//     }

//     if (++stepTimer >= pattData[patternIter].stepPause)
//     {
//         stepTimer = 0; // to next step
//         if (++stepIter >= getPattLength())
//         {
//             stepIter = 0; // to next pattern
//             if (++patternIter >= numPatterns)
//                 patternIter = 0; // reset cycle
//         }
//     }
// }

// void LightPlayer2::updateSubOnOnly() // writes only to onLt
// {
//     Light *pBase = pLt0 + gridCols * row0 + col0;
//     for (int r = 0; r < rows; ++r)
//     {
//         if (r + row0 < 0) continue;
//         if (r + row0 >= gridRows) break;

//         Light *pRow = pBase + r * gridCols;
//         for (int c = 0; c < cols; ++c)
//         {
//             if (c + col0 < 0) continue;
//             if (c + col0 >= gridCols) break;
//             if (getState(r * cols + c))
//                 *(pRow + c) = onLt;
//         }
//     }

//     if (++stepTimer >= pattData[patternIter].stepPause)
//     {
//         stepTimer = 0; // to next step
//         if (++stepIter >= getPattLength())
//         {
//             stepIter = 0; // to next pattern
//             if (++patternIter >= numPatterns)
//                 patternIter = 0; // reset cycle
//         }
//     }
// }

void LightPlayer2::updateSubOnOnly()// writes only to onLt
{
    Light *pBase = pLt0 + gridCols * row0 + col0;
    for (int r = 0; r < rows; ++r)
    {
        if (r + row0 < 0) continue;
        if (r + row0 >= gridRows) break;

        Light *pRow = pBase + r * gridCols;
        for (int c = 0; c < cols; ++c)
        {
            if (c + col0 < 0) continue;
            if (c + col0 >= gridCols) break;
            if (getState(r * cols + c))
                *(pRow + c) = onLt;
        }
    }

    //   takeStep();
}

unsigned int LightPlayer2::getPattLength() const
{
    const unsigned int funcIdx = pattData[patternIter].funcIndex;

    if (funcIdx == 0) return 1; // pause pattern
    if (funcIdx >= 1 && funcIdx <= 5) return numLts;
    if (funcIdx == 6) return pattData[patternIter].param; // alternateBlink
    if (funcIdx == 7) return pattData[patternIter].param; // checkerBlink
    if (funcIdx == 10 || funcIdx == 11) return cols;      // scrollCol
    if (funcIdx == 12 || funcIdx == 13) return rows;      // scrollRow
    if (funcIdx == 14 || funcIdx == 15) return cols / 2;  // BoxIn, BoxOut
    if (funcIdx == 16) return rows + cols;                // scrollDiagonal
    if (funcIdx == 80) return static_cast<uint8_t>((cols + rows) / 4);// scrollRingOut

    // Fill from top uses upper 8 bits to store the fill-to value
    if (funcIdx == 31 || funcIdx == 32 || funcIdx == 33 || funcIdx == 34) return
        getUpperBitsValue(pattData[patternIter].param, 8);

    return 1;
}

bool LightPlayer2::getState(unsigned int n) const
{
    unsigned int funcIdx = pattData[patternIter].funcIndex;
    unsigned int param = pattData[patternIter].param;

    switch (funcIdx)
    {
        case 0: return false; // a "pause" between patterns
        case 1: return scrollToRight(n, param);
        case 2: return scrollToLeft(n, param);
        case 3: return fillFromRight(n);
        case 4: return fillFromLeft(n);
        case 5: return crissCross(n, param);
        case 6: return alternateBlink(n);
        case 7: return checkerBlink(n);
            // 2d patterns
        case 10: return scrollColToRight(n);
        case 11: return scrollColToLeft(n);
        case 12: return scrollRowToBottom(n);
        case 13: return scrollRowToTop(n);
        case 14: return scrollBoxIn(n);
        case 15: return scrollBoxOut(n);
        case 16: return scrollDiagonal(n, param);
        case 80: return scrollRingOut(n);

            // New ones
        case 31:
        {
            const auto toFill = getLowerBitsValue(param, 8);
            const auto toRow = getUpperBitsValue(param, 8);
            return fillColumnFromTop(n, toFill, toRow);
        }
        case 32:
        {
            const auto toFill = getLowerBitsValue(param, 8);
            const auto toRow = getUpperBitsValue(param, 8);
            return unfillColumnFromTop(n, toFill, toRow);
        }
        case 33:
        {
            const auto toFill = getLowerBitsValue(param, 8);
            const auto toRow = getUpperBitsValue(param, 8);
            return fillColumnFromBottom(n, toFill, toRow);
        }
        case 34:
        {
            const auto toFill = getLowerBitsValue(param, 8);
            const auto toRow = getUpperBitsValue(param, 8);
            return unfillColumnFromBottom(n, toFill, toRow);
        }

        default: return false; // offLight
    }

    return false; // offLight
}

bool LightPlayer2::scrollToRight(unsigned int n, unsigned int numInGroup) const
// returns state assignment
{
    return (n >= stepIter && n < stepIter + numInGroup);
}

bool LightPlayer2::scrollToLeft(unsigned int n, unsigned int numInGroup) const
// returns state assignment
{
    return (n <= numLts - 1 - stepIter) && (
        n + numInGroup > numLts - 1 - stepIter);
}

bool LightPlayer2::fillFromRight(unsigned int n) const
{
    return (n >= numLts - 1 - stepIter);
}

bool LightPlayer2::fillFromLeft(unsigned int n) const
{
    return (n <= stepIter);
}

bool LightPlayer2::crissCross(unsigned int n, unsigned int numInGroup) const
{
    bool A = (n >= stepIter && n < stepIter + numInGroup);
    bool B = (n <= numLts - 1 - stepIter);
    B = B && (n + numInGroup > numLts - 1 - stepIter);
    return A || B;
}

bool LightPlayer2::alternateBlink(unsigned int n) const
{
    return (n + stepIter) % 2;
}

bool LightPlayer2::checkerBlink(unsigned int n) const
{
    return (n + n / cols + stepIter) % 2;
}

// patterns for 2d
bool LightPlayer2::scrollColToRight(unsigned int n) const
{
    return stepIter == n % cols;
}

bool LightPlayer2::scrollColToLeft(unsigned int n) const
{
    return stepIter == (cols - 1 - n % cols);
}

bool LightPlayer2::scrollRowToBottom(unsigned int n) const
{
    return stepIter == n / cols;
}

bool LightPlayer2::scrollRowToTop(unsigned int n) const
{
    return stepIter == rows - 1 - n / cols;
}

bool LightPlayer2::scrollBoxIn(unsigned int n) const
{
    int Cmax = cols - 1 - stepIter;
    int Rmax = rows - 1 - stepIter;
    int r = n / cols, c = n % cols;

    if ((r == (int) stepIter || r == Rmax) && (
        c >= (int) stepIter && c <= Cmax))
        return true;
    if ((c == (int) stepIter || c == Cmax) && (
        r >= (int) stepIter && r <= Rmax))
        return true;
    return false;
}

bool LightPlayer2::scrollBoxOut(unsigned int n) const
{
    int Cmax = cols / 2 + stepIter;
    int Cmin = cols - 1 - Cmax;
    int Rmax = rows / 2 + stepIter;
    int Rmin = rows - 1 - Rmax;
    int r = n / cols, c = n % cols;

    if ((r == Rmin || r == Rmax) && (c >= Cmin && c <= Cmax)) return true;
    if ((c == Cmin || c == Cmax) && (r >= Rmin && r <= Rmax)) return true;
    return false;
}

// 0 = dn rt, 1 = up lt, 2 = dn lt, 3 = up rt
bool LightPlayer2::scrollDiagonal(unsigned int n, unsigned int Mode) const
{
    int r = n / cols, c = n % cols;
    if (Mode == 0 && (int) stepIter >= r) return c == (int) stepIter - r;
    // dn rt
    if (Mode == 1) return c == cols - 1 - (int) stepIter + rows - 1 - r;
    // up lt
    if (Mode == 2) return c == cols - 1 - (int) stepIter + r; // dn lt
    if (Mode == 3) return c == (int) stepIter + r - rows - 1; // up rt

    return false;
}

bool LightPlayer2::scrollRingOut(unsigned int n)const// 80
{
    //    float RmaxSq = ( cols*cols + rows*rows )*0.25f;
    int r = n / cols, c = n % cols;
    const unsigned int &Param = pattData[patternIter].param;

    //   float Ry = ( rows - 1 + stepIter - r ), Rx = ( cols - 1 + stepIter - c );
    float Ry = (rows / 2 - r), Rx = (cols / 2 - c);
    float RnSq = (Rx * Rx + Ry * Ry) * 0.25f;
    //   if( RnSq > RmaxSq ) return false;// radius too large
    if (RnSq >= (stepIter) * (stepIter) && RnSq < (stepIter + Param) * (stepIter + Param))
        return true;

    return false;
}

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

bool LightPlayer2::fillColumnFromTop(unsigned int n,
    unsigned int colToFill,
    unsigned int toRow) const
{
    int r = n / cols, c = n % cols;
    unsigned int bitShifted = (1 << c) & colToFill;
    return bitShifted && r <= stepIter;
}

bool LightPlayer2::unfillColumnFromTop(unsigned int n,
    unsigned int colToFill,
    unsigned int toRow) const
{
    int r = n / cols, c = n % cols;
    // const auto pattLen = getPattLength();
    unsigned int bitShifted = (1 << c) & colToFill;
    return bitShifted && (toRow - stepIter - 1) >= r;
}

bool LightPlayer2::fillColumnFromBottom(unsigned int n,
    unsigned int colToFill,
    unsigned int toRow) const
{
    int r = n / cols, c = n % cols;
    unsigned int bitShifted = (1 << c) & colToFill;
    return bitShifted && r >= (rows - stepIter - 1);
}

bool LightPlayer2::unfillColumnFromBottom(unsigned int n,
    unsigned int colToFill,
    unsigned int toRow) const
{
    int r = n / cols, c = n % cols;
    // const auto pattLen = getPattLength();
    unsigned int bitShifted = (1 << c) & colToFill;
    return bitShifted && r >= (toRow + stepIter);
}

// alternate display
void LightPlayer2::updateAsEq(float *pVal)const// cols elements is assumed
{
    int numOn = 0;
    Light *pBase = pLt0 + gridCols * row0 + col0;

    for (int c = 0; c < cols; ++c)
    {
        numOn = pVal[c] * (rows - 1);
        if (numOn < 0) numOn *= -1;// amplitude only
        if (numOn >= rows) numOn = rows - 1;// limit

        Light *pLt = pBase + (rows - 1) * gridCols + c;// start at bottom of column
        for (int n = 0; n < numOn; ++n)
        {
            *pLt = onLt;
            pLt -= gridCols;// up 1 row
        }
        // offLt?
    }
}
