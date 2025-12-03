#ifndef SWITCHSPST_H
#define SWITCHSPST_H

#include "Arduino.h"

// A single pole single throw ( on or off ) switch
class switchSPST
{
public:
    int pinID = 0;
    bool isClosed = false;// open circuit by default
    bool wasClosed = false;// event detection
    // lastNow for de bounce
    bool lastNow = false;

    // de bounce timing
    float tWait = 0.3f;
    float tTimer = tWait;// stable value
    // event handling
    void (*onClose)(void) = nullptr;
    void (*onOpen)(void) = nullptr;

    void init(int PinID, float t_Wait, bool iClosed = false)
    {
        pinID = PinID;
        pinMode(pinID, INPUT_PULLUP);
        tWait = t_Wait;
        tTimer = tWait;// stable
        isClosed = wasClosed = lastNow = iClosed;
    }

    void update(float dt)
    {
        wasClosed = isClosed;

        bool now = (digitalRead(pinID) == LOW);
        if (now != lastNow) tTimer = 0.0f;// reset timer when state has changed
        if (tTimer < tWait)
        {
            tTimer += dt;
            if (tTimer >= tWait)// Ding!           
                isClosed = now;
        }

        lastNow = now;// for next call

        // event handling. When isClosed != wasClosed
        if (onClose && (isClosed && !wasClosed)) onClose();// just closed
        else if (onOpen && (!isClosed && wasClosed)) onOpen();// just opened
    }

    // event polling: 0 = no event, +1 = press, -1 = release
    int pollEvent()const
    {
        if (isClosed == wasClosed) return 0;// no change
        if (isClosed && !wasClosed) return 1;// just closed
        if (!isClosed && wasClosed) return -1;// just opened
        return 0;// should not get here
    }
};

#endif // SWITCHSPST_H