#ifndef LIGHTPLAYER2_H
#define LIGHTPLAYER2_H

#include "Light.h"

class LightPlayer2
{
    public:
        // new iteration scheme
    unsigned int numPatterns = 1;// size of storage arrays
    unsigned int patternIter = 0;// 0 to numPatterns. This is index into arrays

    const unsigned int* stepPause = nullptr;// assign to external array. 1 element per pattern.
    unsigned int stepTimer = 0;// timer for stepIter incrementation

    const unsigned int* patternLength = nullptr;// assign to external array. 1 element per pattern.
    unsigned int stepIter = 0;// 0 to patternLength

    const unsigned int* funcIndex = nullptr;// for use in switch within getState()
    const unsigned int* funcParam = nullptr;// for numInGroup or other or none

    void init( Light& r_Lt0, unsigned int Rows, unsigned int Cols );
    void setArrays( unsigned int NumPatterns, const unsigned int* StepPause,
        const unsigned int* PatternLength, const unsigned int* FuncIndex, const unsigned int* FuncParam );

    void update( const Light& onLt, const Light& offLt );// assign as desired
    bool getState( unsigned int n )const;

    // pattern functions indexed to in switch within getState
    bool scrollToRight( unsigned int n, unsigned int numInGroup )const;// returns state assignment
    bool scrollToLeft( unsigned int n, unsigned int numInGroup )const;// returns state assignment

    bool fillFromRight( unsigned int n )const;
    bool fillFromLeft( unsigned int n )const;

    bool crissCross( unsigned int n, unsigned int numInGroup )const;
    bool alternateBlink( unsigned int n )const;

    LightPlayer2();
    ~LightPlayer2();

    protected:
    Light* pLt0 = nullptr;
    unsigned int rows = 1, cols = 1;

    // dependent. For convenience in functions
    unsigned int numLts = 1;// numLts = rows*cols

    private:
};

#endif // LIGHTPLAYER2_H
