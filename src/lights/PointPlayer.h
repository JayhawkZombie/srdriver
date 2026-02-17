
#ifndef POINTPLAYER_H
#define POINTPLAYER_H

#include "Light.h"
// #include "../FileParser.h"
#include "vec2f.h"

// A zoomie! It leaves a faded trail and times out
class PointPlayer
{
public:
    Light *pLt0 = nullptr;// to LightArr
    int gridCols = 1, gridRows = 1;// bounding grid
    void bindToGrid(Light *p_Lt0, int GridRows, int GridCols)
    {
        pLt0 = p_Lt0; gridRows = GridRows; gridCols = GridCols;
    }

    bool isPlaying = false;

    // zoomie traits
    vec2f pos0, pos;// vel;
    float speed = 10.0f;// leds per second
    Light ptColor;
    //   float tPeriod = 3.0f, tFade = 1.0f, tElap = 0.0f;
       // a path to visit on grid only
    uint8_t *pathX = nullptr, *pathY = nullptr;// parallel arrays
    uint8_t numPoints = 0, currPoint = 0;// nextPoint = 0;
    float currLength = 0.0f;// distance to next point
    float fadeLength = 8.0f;// leaves a trail
    vec2f uCurr;// uPrev;// assign on switch over
    // Grow from a point and shrink to a point
    int growState = 0;// 1: growing, 0: Normal (in use), -1: shrinking
    float growLength = 0.0f;// for both growing and shrinking

    void assignPath(const char *fileName);
    void setup(uint8_t &pathX0, uint8_t &pathY0, uint8_t NumPoints, float speed, Light PtColor);
    void Start();
    void Stop();

    bool update(float dt);// true if animating 
    void draw()const;// update() = time update
    // try Bresenham's algorithm
    void draw2()const;// for all segments
    // per segment
    void drawLine(int8_t x0, int8_t y0, int8_t x1, int8_t y1, Light color)const;
    // helper to rasterize

    void draw3()const;

    PointPlayer() {}
    ~PointPlayer() {}
};

vec2f get_dIter(vec2f line);// step along line

#endif // POINTPLAYER_H
