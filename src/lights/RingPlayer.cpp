#include "RingPlayer.h"

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
    float Rf = R0 + ringWidth;

    for (int n = 0; n < rows * cols; ++n)
    {
        float r = n / cols, c = n % cols;
        float Ry = (fRowC - r), Rx = (fColC - c);
        float RnSq = (Rx * Rx + Ry * Ry) * 0.25f;

        float Rn = sqrtf(RnSq);

        // inside or outside of ring = no draw
        if (Rn < R0 || Rn > Rf)
            continue;

        // within ring R0 <= Rn < Rf
        LtAssigned = true;
        float U = 2.0f * (Rn - R0) / ringWidth;// if R0 < Rn < Rmid
        float Rmid = 0.5f * (R0 + Rf);
        //  float U = 2.0f*( Rn - Rmid )/ringWidth;// if R0 < Rn < Rmid
        if (Rn > Rmid) U = 2.0f * (Rf - Rn) / ringWidth;

        // apply fade
        float fadeU = 1.0f;// no fade
        if (Rn > fadeRadius)
        {
            fadeU = static_cast<float>(fadeRadius + fadeWidth - Rn) / static_cast<float>(fadeWidth);
            if (fadeU < 0.0f) fadeU = 0.0f;// last frame over step
        }

        U *= Amp * fadeU;
        float fadeIn = 1.0f - U;
        Light &currLt = pLt0[n];
        // interpolate
        float fr = U * hiLt.r + fadeIn * currLt.r;
        float fg = U * hiLt.g + fadeIn * currLt.g;
        float fb = U * hiLt.b + fadeIn * currLt.b;

        currLt = Light(fr, fg, fb);
    }// end for each Light

    if (!LtAssigned || (R0 >= fadeRadius + fadeWidth))// terminate pattern
        isPlaying = false;
}

void RingPlayer::updateWave(float dt)
{
    if (!isPlaying) return;

    bool LtAssigned = false;// pattern ends when no Light is assigned
    tElap += dt;
    if (!isRadiating) stopTime += dt;

    float R0 = ringSpeed * tElap;
    if (R0 > fadeRadius + fadeWidth) R0 = fadeRadius + fadeWidth;// stay at the limit
    float wvLen = ringWidth;
    float rotFreq = 6.283f * ringSpeed / wvLen;
    float K = 6.283f / wvLen;
    // U = sinf( 2*PI( Rn/wvLen - tElap*freq ) );

    for (int n = 0; n < rows * cols; ++n)
    {
        float r = n / cols, c = n % cols;
        float Ry = (fRowC - r), Rx = (fColC - c);
        float RnSq = (Rx * Rx + Ry * Ry) * 0.25f;

        float Rn = sqrtf(RnSq);
        if (Rn > R0) continue;// wave must spread
        if (Rn > fadeRadius + fadeWidth) continue;// out of range
        if (!isRadiating && Rn < ringSpeed * stopTime) continue;// not writing to expanding core

        // all within draw
        LtAssigned = true;
        // float U = -Amp * sinf(K * Rn - rotFreq * tElap);// outward traveling wave
        float U = -Amp * sinf(K * Rn - direction * rotFreq * tElap);// outward traveling wave
        // apply fade
        float fadeU = 1.0f;// no fade
        if (Rn > fadeRadius)
        {
            fadeU = static_cast<float>(fadeRadius + fadeWidth - Rn) / static_cast<float>(fadeWidth);
            if (fadeU < 0.0f) fadeU = 0.0f;// last frame over step
        }

        U *= fadeU;
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

    if (!LtAssigned)// animation complete
        isPlaying = false;
}
