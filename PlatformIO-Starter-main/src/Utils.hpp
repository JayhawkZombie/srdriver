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
