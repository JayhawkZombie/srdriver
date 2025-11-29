#ifndef LIGHTPLAYER2_H
#define LIGHTPLAYER2_H

#include "Light.h"
#include <FastLED.h>

// a player for presenting procedural patterns in a specified order
struct patternData// for each pattern in the sequence to be played
{
public:
    unsigned int funcIndex = 0;// which pattern
    unsigned int stepPause = 1;// between each step in the present pattern. To slow animation
    unsigned int param = 0;// varying purpose. see notes for each case in getPattLength() and getState()
    // convenient init
    void init( unsigned int fIdx, unsigned int StepPause = 1, unsigned int Param = 0 )
    { funcIndex = fIdx; stepPause = StepPause; param = Param; }
};

class LightPlayer2 {
public:
    // new iteration scheme
    unsigned int numPatterns = 1; // number of patterns in the sequence
    unsigned int patternIter = 0;
    // 0 to numPatterns. This is index into pattData array below

    // 1 instance per pattern in the sequence
    const patternData *pattData = nullptr;
    // array to be provided on Arduino for this data

    unsigned int stepTimer = 0; // timer for stepIter incrementation
    unsigned int stepIter = 0; // 0 to patternLength

    bool doRepeatSeq = true;
    bool playSinglePattern = false;
    void setToPlaySinglePattern(bool playSingle);
    void firePattern(unsigned int pattIdx);
    bool isPlayingSinglePattern() const {
        // The patterns are always set with stepIter = getPattLength(), and expire in the same state
        Serial.println("isPlayingSinglePattern" + String(playSinglePattern) + " " + String(stepIter) + " " + String(getPattLength()));
        return playSinglePattern && !(stepIter >= getPattLength());
    }

    // new. Lights are members not passed as arguments
    Light onLt, offLt;
    bool drawOffLt = true;

    // new. Find pattern length
    unsigned int getPattLength() const; // lookup for each funcIndex

    // Arduino: Lights[NUM_LEDS]  NUM_LEDS = Rows*Cols     patternData[]            patternData size
    void init(Light &r_Lt0,
        int Rows,
        int Cols,
        const patternData &rPattData,
        unsigned int NumPatterns);

    // 1st player draws the off color
    void takeStep();
    void update();// assign as desired
    // drawMode = 1: is grid
    void updateIsGrid();
    void updateIsGridOnOnly();// writes only to onLt
    // for use as 2nd player in sub rect
    void updateSub();// draw over background
    void updateSubOnOnly();// writes only to onLt. 1st player assign of others stand

    bool getState(unsigned int n) const;
    // of each light (on/off) in the draw of all numLts

    // simple pattern to fill all lights
    bool fillAllLights(unsigned int n, unsigned int param) const;

    // pattern functions indexed to in switch within getState
    bool scrollToRight(unsigned int n, unsigned int numInGroup) const;
    // returns state assignment
    bool scrollToLeft(unsigned int n, unsigned int numInGroup) const;
    // returns state assignment
    bool fillFromRight(unsigned int n) const;
    bool fillFromLeft(unsigned int n) const;
    bool crissCross(unsigned int n, unsigned int numInGroup) const;
    bool alternateBlink(unsigned int n) const;
    bool checkerBlink(unsigned int n) const; // checker board fill

    // patterns for 2d
    bool scrollColToRight(unsigned int n) const;
    bool scrollColToLeft(unsigned int n) const;
    bool scrollRowToBottom(unsigned int n) const;
    bool scrollRowToTop(unsigned int n) const;
    bool scrollBoxIn(unsigned int n) const;
    bool scrollBoxOut(unsigned int n) const;
    // Mode: 0 = dn rt, 1 = up lt, 2 = dn lt, 3 = up lt
    bool scrollDiagonal(unsigned int n, unsigned int Mode) const;
    bool scrollRingOut(unsigned int n)const;// 80

    // New ones
    bool fillColumnFromTop(unsigned int n, unsigned int colToFill, unsigned int toRow) const;
    bool unfillColumnFromTop(unsigned int n, unsigned int colToFill, unsigned int toRow) const;
    bool fillColumnFromBottom(unsigned int n, unsigned int colToFill, unsigned int toRow) const;
    bool unfillColumnFromBottom(unsigned int n, unsigned int colToFill, unsigned int toRow) const;

    LightPlayer2()
    {}

    ~LightPlayer2()
    {}

    void bindToGrid(Light &r_Lt0, int GridRows, int GridCols);
    // set the target rectangle within a larger array (Grid)
    void setGridBounds(int Row0, int Col0, int GridRows, int GridCols)
    {
        row0 = Row0; col0 = Col0; gridRows = GridRows; gridCols = GridCols; setDrawMode();
    }
    // within same grid
    void setTargetRect(int Rows, int Cols, int Row0, int Col0)
    {
        row0 = Row0; col0 = Col0; rows = Rows; cols = Cols; numLts = rows * cols; setDrawMode();
    }
    // useful getters
    int getRows()const { return rows; }
    int getCols()const { return cols; }
    int getRow0()const { return row0; }
    int getCol0()const { return col0; }
    unsigned int getNumLts()const { return numLts; }
    Light *get_pLt0()const { return pLt0; }
    // setters
    void setRows(int Rows) { rows = Rows; setDrawMode(); }
    void setCols(int Cols) { cols = Cols; setDrawMode(); }
    void setRow0(int Row0) { row0 = Row0; setDrawMode(); }
    void setCol0(int Col0) { col0 = Col0; setDrawMode(); }

    // other use?
    void updateAsEq(float *pVal)const;// cols elements is assumed

// protected:                 // new for me. Not everything is public
    Light *pLt0 = nullptr; // to LightArr on Arduino

    //   unsigned int rows = 1, cols = 1;
    int rows = 1, cols = 1; // dimensions of this array
    int row0 = 0, col0 = 0; // origin in grid
    int gridCols = 1, gridRows = 1; // bounding grid
    // dependent. For convenience in functions
    unsigned int numLts = 1; // numLts = rows*cols

    int drawMode = 3;// 1: is grid, 2: is all in grid, 3: is partly in grid
    void setDrawMode();

private:
};

#endif // LIGHTPLAYER2_H
