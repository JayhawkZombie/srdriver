#include "RingPlayer.h"

// bool RPdata::init(FileParser &FP)
// {
//     int rd = 0, gn = 0, bu = 0;
//     FP >> rd >> gn >> bu;
//     hiLt.setRGB(rd, gn, bu);
//     FP >> rd >> gn >> bu;
//     loLt.setRGB(rd, gn, bu);
//     FP >> ringSpeed;
//     FP >> ringWidth;
//     FP >> fadeRadius;
//     FP >> fadeWidth;
//     FP >> fRowC;
//     FP >> fColC;
//     FP >> Amp;
//     return true;
// }

bool RingPlayer::update(float dt)// true if animating
{
    if (!isPlaying) return false;

    if (onePulse) updatePulse(dt);
    else updateWave(dt);

    return isPlaying;
}

void RingPlayer::updatePulse(float dt)
{
    bool LtAssigned = false;// pattern ends when no Light is assigned
    tElap += dt;

    float R0 = ringSpeed * tElap;
    float R0sq = R0 * R0;
    float RF = R0 + ringWidth;
    float RFsq = RF * RF;

    //    if( Rf + fColC < 0.0f ) return;
    //    if( fColC - Rf > cols ) return;
    //    if( Rf + fRowC < 0.0f ) return;
    //    if( fRowC - Rf > rows ) return;

    for (int n = 0; n < rows * cols; ++n)
    {
        float r = n / cols, c = n % cols;
        float Ry = (fRowC - r), Rx = (fColC - c);
        float RnSq = (Rx * Rx + Ry * Ry) * 0.25f;

        // inside or outside of ring = no draw
        if (RnSq < R0sq || RnSq > RFsq)
            continue;

        float Rn = sqrtf(RnSq);// after continue
        // apply fade
        float fadeU = 1.0f;// no fade        
        if (Rn > fadeRadius)
        {
            fadeU = static_cast<float>(fadeRadius + fadeWidth - Rn) / static_cast<float>(fadeWidth);
            if (fadeU < 0.01f) continue;// last frame over step
        }

        // within ring R0 <= Rn < Rf
        LtAssigned = true;
        float U = 2.0f * (Rn - R0) / ringWidth;// if R0 < Rn < Rmid
        float Rmid = 0.5f * (R0 + RF);
        if (Rn > Rmid) U = 2.0f * (RF - Rn) / ringWidth;
        U *= Amp * fadeU;
        float fadeIn = 1.0f - U;
        Light &currLt = pLt0[n];
        // interpolate
        float fr = U * hiLt.r + fadeIn * currLt.r;
        float fg = U * hiLt.g + fadeIn * currLt.g;
        float fb = U * hiLt.b + fadeIn * currLt.b;

        currLt = Light(fr, fg, fb);
    }// end for each Light

    // new
    if (LtAssigned && !isVisible)
        isVisible = true;// has reached the grid

    if (isVisible && (!LtAssigned || (R0 >= fadeRadius + fadeWidth)))
        isPlaying = false;// animation complete
}

void RingPlayer::updateWave(float dt)
{
    if (!isPlaying) return;

    bool LtAssigned = false;// pattern ends when no Light is assigned
    tElap += dt;
    if (!isRadiating) stopTime += dt;

    float R0 = ringSpeed * tElap;
    // handle centers off grid. Start too late? Wave pops into view
//   if( R0 + fColC < 0.0f ) return;
//    if( fColC - R0 > cols ) return;
//    if( R0 + fRowC < 0.0f ) return;
//    if( fRowC - R0 > rows ) return;

    if (R0 > fadeRadius + fadeWidth) R0 = fadeRadius + fadeWidth;// stay at the limit
    //  float wvLen = ringWidth;
    float rotFreq = 6.283f * ringSpeed / ringWidth;
    float K = 6.283f / ringWidth;
    // new. To delay calling sqrtf()
    float R0sq = R0 * R0;
    float frwSq = (fadeRadius + fadeWidth) * (fadeRadius + fadeWidth);

    for (int n = 0; n < rows * cols; ++n)
    {
        float r = n / cols, c = n % cols;
        float Ry = (fRowC - r), Rx = (fColC - c);
        float RnSq = (Rx * Rx + Ry * Ry) * 0.25f;

        //   float Rn = sqrtf( RnSq );
        //   if( Rn > R0 ) continue;// wave must spread
        //   if( Rn > fadeRadius + fadeWidth ) continue;// out of range
           // cheaper?
        if (RnSq > R0sq) continue;// wave must spread
        if (RnSq > frwSq) continue;// out of range
        // now do it
        float Rn = sqrtf(RnSq);

        if (!isRadiating && Rn < ringSpeed * stopTime) continue;// not writing to expanding core

        // all within draw
     //   LtAssigned = true; not yet there is another continue

        float fadeU = 1.0f;
        if (Rn > fadeRadius)
        {
            fadeU = static_cast<float>(fadeRadius + fadeWidth - Rn) / static_cast<float>(fadeRadius + fadeWidth);
            if (fadeU < 0.01f) continue;
        }

        // all within draw
        LtAssigned = true;
        float U = -Amp * sinf(K * Rn - direction * rotFreq * tElap);// traveling wave        
        U *= fadeU;// apply fade
        float fadeIn = (U > 0.0f) ? 1.0f - U : 1.0f + U;
        Light &currLt = pLt0[n];
        // interpolate
        float fr = fadeIn * currLt.r;
        float fg = fadeIn * currLt.g;
        float fb = fadeIn * currLt.b;
        if (U > 0.0f)
        {
            fr += U * hiLt.r;
            fg += U * hiLt.g;
            fb += U * hiLt.b;
        }
        else
        {
            fr -= U * loLt.r;// - because U < 0
            fg -= U * loLt.g;
            fb -= U * loLt.b;
        }

        currLt = Light(fr, fg, fb);
    }// end for each Light

    // new
    if (LtAssigned && !isVisible) isVisible = true;

    if (isVisible && !LtAssigned)// animation complete
        isPlaying = false;
}

// static methods. FloatAll stores R0, RF, fadeRate for each player
// LtAssAll stores LtAssigned for each player
void RingPlayer::updatePulseAll(RingPlayer *pRP, int numRP, float dt, float *FloatAll, bool *LtAssAll)
{
    // MUST have common member values: rows, cols, pLt0
    int Rows = pRP[0].rows;
    int Cols = pRP[0].cols;
    Light *p_Lt0 = pRP[0].pLt0;
    for (int i = 1; i < numRP; ++i)
    {
        if (!pRP[i].isPlaying) continue;
        if (pRP[i].rows != Rows) return;
        if (pRP[i].cols != Cols) return;
        if (pRP[i].pLt0 != p_Lt0) return;
    }

    for (int k = 0; k < numRP; ++k)
    {
        if (!pRP[k].isPlaying) continue;
        LtAssAll[k] = false;// pattern ends when no Light is assigned
        pRP[k].tElap += dt;

        float R0 = pRP[k].ringSpeed * pRP[k].tElap;// by RP
        FloatAll[3 * k] = R0;// by RP
        //  float R0sq = R0*R0;
        float RF = R0 + pRP[k].ringWidth;// by RP
        FloatAll[3 * k + 1] = R0 + pRP[k].ringWidth;// by RP
        //  float RFsq = RF*RF;
        float fadeRate = 2.0f / pRP[k].ringWidth;// by RP
        FloatAll[3 * k + 2] = fadeRate;
    }

    for (int n = 0; n < Rows * Cols; ++n)
    {
        float r = n / Cols, c = n % Cols;

        float U = 0.0f;// buildup values below
        float fRedHi = 0.0f, fGreenHi = 0.0f, fBlueHi = 0.0f;

        // each player
        for (int k = 0; k < numRP; ++k)
        {
            if (!pRP[k].isPlaying) continue;
            float Ry = (pRP[k].fRowC - r), Rx = (pRP[k].fColC - c);// by RP
            float RnSq = (Rx * Rx + Ry * Ry) * 0.25f;

            float R0 = FloatAll[3 * k];
            float RF = FloatAll[3 * k + 1];
            // inside or outside of ring = no draw
            if (RnSq < R0 * R0 || RnSq > RF * RF)
                continue;

            float Rnk = sqrtf(RnSq);// after continue
            //    FloatAll[ 4*k + 3 ] = Rn;
                // apply fade
            float fadeU = 1.0f;// no fade        
            if (Rnk > pRP[k].fadeRadius)// by RP
            {
                fadeU = static_cast<float>(pRP[k].fadeRadius + pRP[k].fadeWidth - Rnk) / static_cast<float>(pRP[k].fadeWidth);
                if (fadeU < 0.01f) continue;// last frame over step
            }

            // within ring R0 <= Rn < Rf
        //  LtAssigned = true;
            LtAssAll[k] = true;
            float fadeRate = FloatAll[3 * k + 2];
            float dU = fadeRate * (Rnk - R0);// if R0 < Rn < Rmid
            float Rmid = 0.5f * (R0 + RF);
            if (Rnk > Rmid) dU = fadeRate * (RF - Rnk);
            // buildup U
            dU *= pRP[k].Amp * fadeU;// by RP
            U += dU;
            // ?? buildup HiLt ??
            fRedHi += dU * (float) pRP[k].hiLt.r;// by RP
            fGreenHi += dU * (float) pRP[k].hiLt.g;
            fBlueHi += dU * (float) pRP[k].hiLt.b;
            //   Light HiLt = pRP[k].hiLt;// by RP
        }// end by RP

        float fadeIn = 1.0f - U;
        Light &currLt = p_Lt0[n];
        // interpolate
        float fr = fRedHi + fadeIn * (float) currLt.r;
        float fg = fGreenHi + fadeIn * (float) currLt.g;
        float fb = fBlueHi + fadeIn * (float) currLt.b;
        currLt = Light(fr, fg, fb);

        //  float fr = U*HiLt.r + fadeIn*currLt.r;
        //  float fg = U*HiLt.g + fadeIn*currLt.g;
        //  float fb = U*HiLt.b + fadeIn*currLt.b;
        //  currLt = Light( fr, fg, fb );
    }// end for each Light

    // new
    for (int k = 0; k < numRP; ++k)
    {
        if (LtAssAll[k] && !pRP[k].isVisible)
            pRP[k].isVisible = true;// has reached the grid

        float R0 = FloatAll[3 * k];
        if (pRP[k].isVisible && (!LtAssAll[k] || (R0 >= pRP[k].fadeRadius + pRP[k].fadeWidth)))
            pRP[k].isPlaying = false;// animation complete
    }
}