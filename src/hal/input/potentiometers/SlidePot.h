#ifndef SLIDEPOT_H
#define SLIDEPOT_H

#include "Arduino.h"

// A slide potentiometer
class slidePot
{
public:
    int pinID = 0;
    int steadyVal = 0;
    int minDiff = 1;
    int bitShift = 3;
    // require inVal to exceed minDiff several times in a row
    int bumpLimit = 3, bumpCnt = 0;

    void setup(int PinID, int BitShift, int MinDiff, int BumpLimit)
    {
        pinID = PinID;
        bitShift = BitShift;
        bumpLimit = BumpLimit;
        bumpCnt = 0;
        minDiff = MinDiff;
        steadyVal = 0;
    }

    slidePot(int PinID, int BitShift, int MinDiff, int BumpLimit) { setup(PinID, BitShift, MinDiff, BumpLimit); }
    slidePot() {}

    int update()
    {
        int inNow = analogRead(pinID);
        int U = inNow >> bitShift;
        if ((U >= steadyVal + minDiff) || (U <= steadyVal - minDiff))
        {
            ++bumpCnt;
            if (bumpCnt >= bumpLimit)
            {
                steadyVal = U;
                bumpCnt = 0;// reset
            }
        }
        else
        {
            bumpCnt = 0;// reset
        }


        return steadyVal;
    }
};

#endif // SLIDEPOT_H
