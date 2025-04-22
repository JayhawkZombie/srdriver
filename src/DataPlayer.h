#ifndef DATAPLAYER_H
#define DATAPLAYER_H

#include "Light.h"
#include "utility/bitArray.h"

class DataPlayer
{
    protected:
    Light* pLt0 = nullptr;
    int rows = 1, cols = 1;
    int row0 = 0, col0 = 0;// origin in grid
    int gridCols = 1, gridRows = 1;// bounding grid
    // dependent. For convenience in functions
    unsigned int numLts = 1;// numLts = rows*cols
    int drawMode = 3;// 1: is grid, 2: is all in grid, 3: is partly in grid
    void updateIsGrid();// 1
    void updateAllIn();// 2
    void updatePartlyIn();// 3

    public:
    Light Lt[16];// use 0 and 1 for 2 colors
    bool drawOff = true;// draw both if true or draw only onLt if false
    bool fadeAlong = false;// fade each frame into the next
    Light updateFade( unsigned int n )const;

    // play pause seek functions
    bool isPlaying = true;
    void reStart(){ stepTimer = stepIter = 0; }
    void prevImage();
    void nextImage();
    void showImage( unsigned int n );// 0 to numSteps - 1

    // storage for bitArray
    uint8_t* pStateData = nullptr;
    unsigned int stateDataSz = 0;
    // newest
    bitArray BA;// for bitwise storage of above stateData
    uint8_t numColors = 2;// 2, 4 or 16

    // transform image
    bool flipX = false, flipY = false;
    void flipX_AllIn();// for drawMode = 1 or 2
    void flipY_AllIn();// for drawMode = 1 or 2


    // state management
    unsigned int stepTimer = 0, stepPause = 1;// timer for stepIter incrementation
    unsigned int stepIter = 0, numSteps = 1;

    void update();

    Light getState( unsigned int n )const;

    // test or utility
    void showColors()const;// display in order 0 to numLts - 1

    int getRows()const{ return rows; }
    int getCols()const{ return cols; }
    unsigned int getNumLts()const{ return numLts; }

    void init( Light& r_Lt0, int Rows, int Cols, uint8_t& r_StateData, unsigned int DataSz, uint8_t NumColors );

    void setGridBounds( int Row0, int Col0, int GridRows, int GridCols );

    DataPlayer(){}
    ~DataPlayer(){}

    private:
};

#endif // DATAPLAYER_H
