#include "PulsePlayer.h"
#include <cmath>

void PulsePlayer::init(Light &r_Lt0, int NumLts, Light HiLt, int W_pulse, float Speed, bool DoRepeat)
{
    pLt0 = &r_Lt0;
    numLts = NumLts;
    hfW = W_pulse / 2;
    // Prevent division by zero - ensure speed is never exactly 0
    if (fabsf(Speed) < 0.001f) {
        speed = Speed >= 0.0f ? 0.001f : -0.001f;
    } else {
        speed = Speed;
    }
    fRd = (float) HiLt.r; fGn = (float) HiLt.g; fBu = (float) HiLt.b;
    doRepeat = DoRepeat;
    if (doRepeat)
    {
        Start();
    }
    else// park at end of array
    {
        // Safe division - speed is guaranteed to be non-zero at this point
        tElap = (numLts + hfW) / speed;// off left end
        if (speed < 0.0f) tElap *= -1.0f;// so tElap > 0
    }

    //  if( speed > 0.0f )
   //       tElap = doRepeat ? -hfW/speed : ( numLts + hfW )/speed + 1.0f;// off right end
   //   else
    //      tElap = doRepeat ? -1.0f*( numLts + hfW )/speed : hfW/speed;// off left end


}

void PulsePlayer::update(float dt)
{
    // Safety check - ensure pLt0 is valid
    if (!pLt0 || numLts <= 0) {
        return;
    }
    
    // Prevent division by zero
    if (fabsf(speed) < 0.001f) {
        return;  // Can't update with zero speed
    }
    
    if (speed < 0.0f) return updateLeft(dt);

    if (tElap * speed < numLts + hfW)
    {
        tElap += dt;
        if (doRepeat && tElap * speed >= numLts + hfW)
            tElap = -hfW / speed;// off of left end

        int nc = tElap * speed;
        if (nc + hfW < 0 || nc - hfW >= numLts)// off left or right end
            return;

        // draw pulse
        float u = 0.0f;
        for (int n = nc - hfW; n < nc + hfW; ++n)
        {
            if (n < 0) continue;// not on grid yet
            if (n >= numLts) break;// have left the grid

            if (n < nc)
                u = 1.0f - (float) (nc - n) / (float) hfW;// u: 0 to 1
            else
                u = 1.0f - (float) (n - nc) / (float) hfW;// u: 1 to 0

            // Bounds check before pointer arithmetic
            if (n >= 0 && n < numLts) {
                *(pLt0 + n) = get_Lt(get_y(u), n);
            }
        }// end draw pulse

    }// end if
}

void PulsePlayer::updateLeft(float dt)// for speed < 0
{
    // Safety check - ensure pLt0 is valid
    if (!pLt0 || numLts <= 0) {
        return;
    }
    
    // Prevent division by zero
    if (fabsf(speed) < 0.001f) {
        return;  // Can't update with zero speed
    }
    
    if (numLts + hfW + tElap * speed + 1 >= 0)
    {
        tElap += dt;
        int nc = numLts + tElap * speed;
        if (doRepeat && nc + hfW < 0)
            tElap = hfW / speed;// off of right end

        // same as for speed > 0 from here
        if (nc + hfW < 0 || nc - hfW >= numLts)// off left or right end
            return;

        // draw pulse
        float u = 0.0f;
        for (int n = nc - hfW; n < nc + hfW; ++n)
        {
            if (n < 0) continue;// not on grid yet
            if (n >= numLts) break;// have left the grid

            if (n < nc)
                u = 1.0f - (float) (nc - n) / (float) hfW;// u: 0 to 1
            else
                u = 1.0f - (float) (n - nc) / (float) hfW;// u: 1 to 0

            // Bounds check before pointer arithmetic
            if (n >= 0 && n < numLts) {
                *(pLt0 + n) = get_Lt(get_y(u), n);
            }
        }// end draw pulse

    }// end if
}

float PulsePlayer::get_y(float u)const// switches on funcIdx
{
    switch (funcIdx)
    {
        case 0: return u;// yp(0) = yp(1) = 1 a line
        case 1: return u * (2.0f - u);// yp(0) = 2, yp(1) = 0 quadratic
        case 2: return u * u * (3.0f - 2.0f * u);// yp(0) = yp(1) = 0 cubic
    }

    return 0.0f;
}

Light PulsePlayer::get_Lt(float y, unsigned int nLo)const// interpolate  between existing color and hiLt
{
    // Safety check - ensure pLt0 is valid and nLo is within bounds
    if (!pLt0 || nLo >= (unsigned int)numLts) {
        // Return a safe default color if bounds check fails
        return Light(0, 0, 0);
    }
    
    const Light &ExLt = *(pLt0 + nLo);
    float rLo = (float) ExLt.r, gLo = (float) ExLt.g, bLo = (float) ExLt.b;
    float fr = (1.0f - y) * rLo + y * fRd;
    float fg = (1.0f - y) * gLo + y * fGn;
    float fb = (1.0f - y) * bLo + y * fBu;
    return Light(fr, fg, fb);
}

void PulsePlayer::Start()
{
    // Prevent division by zero - speed should never be 0 due to init() check, but be defensive
    if (fabsf(speed) < 0.001f) {
        // If speed is somehow 0, set a safe default
        speed = speed >= 0.0f ? 0.001f : -0.001f;
    }
    if (speed > 0.0f)
        tElap = -hfW / speed;
    else
        tElap = hfW / speed;
}