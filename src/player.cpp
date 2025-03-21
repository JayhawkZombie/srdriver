#include "player.hpp"
#include "Globals.h" // Include any necessary headers for LightPlayer2 and patternData
#include "LightPlayer2.h"

LightPlayer2 LtPlay2; // Declare the LightPlayer2 instance
patternData pattData[16]; // Declare the patternData array

void player_setup() {
    // Initialize the pattern data
    pattData[0].init(1, 1, 5);
    pattData[1].init(2, 1, 3);
    pattData[2].init(7, 8, 10); // checkerboard blink
    pattData[3].init(100, 20, 1); // pattern 100 persists for 20 frames
    pattData[4].init(3, 1, 1);
    pattData[5].init(4, 1, 1);
    pattData[6].init(5, 1, 3);
    pattData[7].init(6, 8, 12);
    pattData[8].init(10, 2, 1);
    pattData[9].init(11, 2, 1);
    pattData[10].init(12, 2, 1);
    pattData[11].init(13, 2, 1);
    pattData[12].init(14, 4, 1);
    pattData[13].init(15, 4, 1);
    pattData[14].init(16, 2, 1);
    pattData[15].init(0, 30, 1); // 30 x loop() calls pause before replay

    // Initialize LightPlayer2
    LtPlay2.init(LightArr[0], 8, 8, pattData[0], 15);
}

void player_loop() {
    // Implement any logic needed for the LightPlayer2 in the loop
    // For example, you can call LtPlay2.update() here
}
