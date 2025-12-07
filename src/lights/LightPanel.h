#ifndef LIGHTPANEL_H
#define LIGHTPANEL_H

#include "Light.h"

// maps from a bounding array of Lights ( pSrc )
// to a same sized array tiled to panels

struct PanelConfig {
    int rows = 8;
    int cols = 8;
    int row0 = 0;
    int col0 = 0;
    int type = 1;
    int rotIdx = 0;
    bool swapTgtRCs = false;
    PanelConfig() = default;
    PanelConfig(int rows, int cols, int row0, int col0, int type, int rotIdx, bool swapTgtRCs)
        : rows(rows), cols(cols), row0(row0), col0(col0), type(type), rotIdx(rotIdx), swapTgtRCs(swapTgtRCs) {}
};

class LightPanel
{
public:
    // location in source array
    Light *pSrc0 = nullptr;
    int srcRows = 8, srcCols = 8;// large bounding grid
    int row0, col0;
    int rows, cols;

    // additional processing
    int type = 1;// no added process. 2 = serpentine order. Reverse odd rows.
    void reverseOddRows(bool inTarget)const;// for type = 2
    // rotate before writing to target array
    int rotIdx = 0;// 0 = no rotation, +1 = 90 cw, -1 = 90 ccw, +-2 = 180 degree
    bool swapTgtRCs = false;// swap role of rows and cols in target
    void updateSideways()const;
    void reverseOddRowsSideways()const;// if swapTgtRCs and type == 2

    // rows == cols only
    void rotateCW()const;// rotate image 90 degrees clockwise
    void rotateCCW()const;// 90 degrees counter clockwise
    // ok if rows != cols
    void rotate180()const;// rotate image 180 degrees

    // location in target array
    Light *pTgt0 = nullptr;// start address for panel
    void init_Src(Light *p_Src0, int SrcRows, int SrcCols)// all must match across panels tiling grid
    {
        pSrc0 = p_Src0; srcRows = SrcRows; srcCols = SrcCols;
    }
    void set_SrcArea(int Rows, int Cols, int Row0, int Col0)
    {
        rows = Rows; cols = Cols; row0 = Row0; col0 = Col0;
    }
    void update()const;// write to target array

    LightPanel() {}
    ~LightPanel() {}

protected:

private:
};

#endif // LIGHTPANEL_H
