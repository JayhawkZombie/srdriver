#pragma once

float easeInOutCubicFloat(float perc)
{
    if (perc < 0.5)
    {
        return 4 * perc * perc * perc;
    }
    else
    {
        float op = -2 * perc + 2;
        return 1 - (op * op * op) / 2;
    }
}

// Float operations are slow on arduino, this should be using
// fixed-point arithmetic with something like fract8 (fraction of 256ths)
template <typename T>
T InterpolateCubicFloat(T minVal, T maxVal, float uneasedFraction)
{
    float easedFloat = easeInOutCubicFloat(uneasedFraction);
    T val = minVal + (easedFloat * maxVal);
    return val;
}
