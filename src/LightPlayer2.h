#ifndef LIGHTPLAYER2_H
#define LIGHTPLAYER2_H

#include "Light.h"
#include "utility/bitArray.h"

struct patternData
{
public:
    unsigned int funcIndex = 0;
    unsigned int stepPause = 1;
    unsigned int param = 0;
    void init( unsigned int fIdx, unsigned int StepPause = 1, unsigned int Param = 0 )
    { funcIndex = fIdx; stepPause = StepPause; param = Param; }
};

class LightPlayer2
{
    public:
        // new iteration scheme
    unsigned int numPatterns = 1;// size of storage arrays
    unsigned int patternIter = 0;// 0 to numPatterns. This is index into arrays

    // new
    const patternData* pattData = nullptr;
    // newer
    uint8_t* pStateData = nullptr;// pattern #100: Length = stateDataSz/numLts
    unsigned int stateDataSz = 0;
    // newest
    bitArray BA;// for bitwise storage of above stateData
    void setStateData( uint8_t* p_StateData, unsigned int DataSz );

    unsigned int stepTimer = 0;// timer for stepIter incrementation
    unsigned int stepIter = 0;// 0 to patternLength

    // new. Find pattern length
    unsigned int getPattLength()const;

    void init( Light& r_Lt0, unsigned int Rows, unsigned int Cols, const patternData& rPattData, unsigned int NumPatterns );

    void update( const Light& onLt, const Light& offLt );// assign as desired
    bool getState( unsigned int n )const;

    // pattern functions indexed to in switch within getState
    bool scrollToRight( unsigned int n, unsigned int numInGroup )const;// returns state assignment
    bool scrollToLeft( unsigned int n, unsigned int numInGroup )const;// returns state assignment

    bool fillFromRight( unsigned int n )const;
    bool fillFromLeft( unsigned int n )const;

    bool crissCross( unsigned int n, unsigned int numInGroup )const;
    bool alternateBlink( unsigned int n )const;
    bool checkerBlink( unsigned int n )const;// checker board fill

    // patterns for 2d
    bool scrollColToRight( unsigned int n, unsigned int numInGroup )const;
    bool scrollColToLeft( unsigned int n, unsigned int numInGroup )const;
    bool scrollRowToBottom( unsigned int n, unsigned int numInGroup )const;
    bool scrollRowToTop( unsigned int n, unsigned int numInGroup )const;
    bool scrollBoxIn( unsigned int n )const;
    bool scrollBoxOut( unsigned int n )const;

    bool scrollDiagonal( unsigned int n )const;

    LightPlayer2(){}
    ~LightPlayer2(){}

    protected:
    Light* pLt0 = nullptr;
    unsigned int rows = 1, cols = 1;

    // dependent. For convenience in functions
    unsigned int numLts = 1;// numLts = rows*cols

    private:
};

#endif // LIGHTPLAYER2_H
