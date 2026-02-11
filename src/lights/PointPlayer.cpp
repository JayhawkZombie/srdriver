#include "PointPlayer.h"

void PointPlayer::assignPath(const char *fileName)
{

}

void PointPlayer::setup(uint8_t &pathX0, uint8_t &pathY0, uint8_t NumPoints, float Speed, Light PtColor)
{
    pathX = &pathX0;
    pathY = &pathY0;
    numPoints = NumPoints;
    speed = Speed;
    ptColor = PtColor;

    Start();
}

void PointPlayer::Start()
{
    currPoint = 0;
    //   nextPoint = 1;
    pos0.x = pos.x = pathX[0];
    pos0.y = pos.y = pathY[0];
    vec2f posf(pathX[1], pathY[1]);
    uCurr = posf - pos0;
    currLength = uCurr.mag();
    uCurr /= currLength;
    pos = pos0;// + ( 1.0f + fadeLength )*uCurr;// start ahead
    //  uPrev = uCurr;    
    //  vel = ( posf - pos0 )*( speed/currLength );
    isPlaying = true;
    growState = 1;
    growLength = 0.0f;
}

void PointPlayer::Stop()
{
    growState = -1;
    growLength = 0.0f;// using as stop distance while not moving
}

bool PointPlayer::update(float dt)// true if animating
{
    if (!isPlaying) return false;

    if (growState == -1)// stopping
    {   // stopping
        growLength += speed * dt;
        if (growLength >= fadeLength)
        {
            isPlaying = false;// stopped
            growState = 0;// may resume at full length
        }

        return true;
    }
    else if (growState == 1)// starting
    {
        growLength += speed * dt;
        if (growLength >= fadeLength)
        {
            growState = 0;
        }
    }

    // update position ( for growState = 0 or 1 )
    pos += uCurr * (speed * dt);
    if ((pos - pos0).mag() >= currLength)// change leg
    {
        currPoint = (1 + currPoint) % numPoints;
        //    uPrev = uCurr;
        pos0.x = pos.x = pathX[currPoint];
        pos0.y = pos.y = pathY[currPoint];
        uint8_t nextPoint = (1 + currPoint) % numPoints;
        vec2f posf(pathX[nextPoint], pathY[nextPoint]);
        uCurr = posf - pos0;
        currLength = uCurr.mag();
        uCurr /= currLength;
        //  vel = ( posf - pos0 )*( speed/currLength );
    }

    return true;
}

void PointPlayer::draw()const
{
    if (!pLt0) return;
    if (!isPlaying) return;

    int r = pos.y, c = pos.x;
    // check bounds?
    if (r >= 0 && r < gridRows && c >= 0 && c < gridCols)
        pLt0[r * gridCols + c] = ptColor;

    // the trail
    vec2f trailIter = pos, iterDir = uCurr;
    vec2f prevPos = pos0;// easy init
    uint8_t currPointIter = currPoint;// easy init
    // calculate uPrev per leg back. It is the new iterDir
    float trailDist = 0.0f;// go to fadeLength
    if (growState == -1) trailDist = growLength;// head start

    while (trailDist < fadeLength)
    {
        trailDist += 0.5f;
        trailIter -= 0.5f * iterDir;
        // may back around multiple corners
        if (iterDir.dot(trailIter - prevPos) < 0.0f)// prevPos slightly over shot by trailIter
        {
            trailIter = prevPos;// at the corner
            // retard currPointIter = point moving back towards
            if (growState == 1)// growing. Stop at start point 0
            {
                if (currPointIter > 0) --currPointIter;
                else break;// growing from start point = pathX[0], pathY[0]
            }
            else if (currPointIter > 0) --currPointIter;
            else currPointIter = numPoints - 1;// keep going
            // new prevPos
            prevPos.x = pathX[currPointIter];
            prevPos.y = pathY[currPointIter];
            // new iterDir  
            iterDir = trailIter - prevPos;
            iterDir /= iterDir.mag();
        }

        int r = trailIter.y, c = trailIter.x;
        // check bounds?
        if (r >= 0 && r < gridRows && c >= 0 && c < gridCols)
        {
            // test: no fade
         //   pLt0[ r*gridCols + c ] = ptColor;
         //   continue;
            // fade
            float U = trailDist / fadeLength;
            float W = 1.0f - U;
            if (W <= 0.0f) break;// fully faded
            int n = r * gridCols + c;
            float fr = W * ptColor.r + U * pLt0[n].r;
            float fg = W * ptColor.g + U * pLt0[n].g;
            float fb = W * ptColor.b + U * pLt0[n].b;
            Light blendLt(fr, fg, fb);
            pLt0[n] = blendLt;
        }
    }
}

// helper to rasterize
vec2f get_dIter(vec2f line)// step along line
{
    bool steep = (line.y * line.y > line.x * line.x);
    // correct if line.x > 0 && line.y > 0
    vec2f dIter = steep ? vec2f(line.x / line.y, 1.0f) : vec2f(1.0f, line.y / line.x);
    if (steep && line.y < 0.0f)
    {
        float ux = -line.x / line.y;
        dIter = vec2f(ux, -1.0f);
    }

    if (!steep && line.x < 0.0f)
    {
        float uy = -line.y / line.x;
        dIter = vec2f(-1.0f, uy);
    }

    return dIter;
}

// raster + fade
void PointPlayer::draw3()const
{
    if (!pLt0) return;
    if (!isPlaying) return;

    // the trail
    uint8_t currPointIter = currPoint;// easy init
    float trailDist = 0.0f;// go to fadeLength
    if (growState == -1) trailDist = growLength;// head start
    // fix ends at integer position
    int nextPoint = (1 + currPoint) % numPoints;
    //  int x0 = floor(pos.x), y0 = floor(pos.y);
    int x0 = pathX[nextPoint], y0 = pathY[nextPoint];
    int xf = pathX[currPoint], yf = pathY[currPoint];
    vec2f P0(x0, y0), Pf(xf, yf);// ends for here
    vec2f line = Pf - P0;
    bool steep = (line.y * line.y > line.x * line.x);
    vec2f dIter = get_dIter(line);
    float stepLength = dIter.mag();// increment trailDist
    float distP0 = (pos - pos0).mag();
    // start at multiple of dIter steps from Pf
    P0 = Pf - ((floor) (distP0 / stepLength)) * dIter;
    x0 = P0.x; y0 = P0.y;// must agree

    while (trailDist < fadeLength)
    {
        //  if( x0 >= 0 && x0 < gridCols && y0 >= 0 && y0 < gridRows )
        //      pLt0[ y0*gridCols + x0 ] = ptColor;   

        for (vec2f iter = P0; (Pf - iter).dot(line) >= 0.0f; iter += dIter)// until iter passes Pf
        {
            trailDist += stepLength;
            if (trailDist >= fadeLength) return;// done!

            int r = floor(iter.y), c = floor(iter.x);
            // bound check
            if (r < 0)
            {
                if (line.y < 0.0f) break; else continue;
            }// break if going up
            else if (r >= gridRows)
            {
                if (line.y > 0.0f) break; else continue;
            }// break if going down
            if (c < 0)
            {
                if (line.x < 0.0f) break; else continue;
            }
            else if (c >= gridCols)
            {
                if (line.x > 0.0f) break; else continue;
            }

            float U = trailDist / fadeLength;// fade factor
            float W = 1.0f - U;

            if (steep)// write to left and right of the line
            {
                int n = r * gridCols + c;
                // vertical case. Write to just 1 Light
                if (x0 == xf)
                {
                    pLt0[n] = Light(W * ptColor.r + U * pLt0[n].r, W * ptColor.g + U * pLt0[n].g, W * ptColor.b + U * pLt0[n].b);
                    continue;
                }
                // weights for Light colors
                float fx = iter.x - c, rfx = 1.0f - fx;// fraction and remaining fraction                
                float Q = rfx * W, R = rfx * U + fx;
                pLt0[n] = Light(Q * ptColor.r + R * pLt0[n].r, Q * ptColor.g + R * pLt0[n].g, Q * ptColor.b + R * pLt0[n].b);
                // 2nd to right since floor went to left            
                if (++c < gridCols)
                {   // apply other fraction
                    ++n;
                    Q = fx * W;
                    R = fx * U + rfx;
                    pLt0[n] = Light(Q * ptColor.r + R * pLt0[n].r, Q * ptColor.g + R * pLt0[n].g, Q * ptColor.b + R * pLt0[n].b);
                }
            }
            else// write above and below
            {
                int n = r * gridCols + c;
                // horizontal case. Write to just 1 Light
                if (y0 == yf)
                {
                    pLt0[n] = Light(W * ptColor.r + U * pLt0[n].r, W * ptColor.g + U * pLt0[n].g, W * ptColor.b + U * pLt0[n].b);
                    continue;
                }
                float fy = iter.y - r, rfy = 1.0f - fy;// fraction and remaining fraction               
                float Q = rfy * W, R = rfy * U + fy;
                pLt0[n] = Light(Q * ptColor.r + R * pLt0[n].r, Q * ptColor.g + R * pLt0[n].g, Q * ptColor.b + R * pLt0[n].b);
                // 2nd below since floor went up            
                if (++r < gridRows)
                {   // apply other fraction
                    n += gridCols;
                    Q = fy * W;
                    R = fy * U + rfy;
                    pLt0[n] = Light(Q * ptColor.r + R * pLt0[n].r, Q * ptColor.g + R * pLt0[n].g, Q * ptColor.b + R * pLt0[n].b);
                }
            }

            //   trailDist += stepLength;
            //   if( trailDist >= fadeLength ) return;// done!

        }// end for

        // will write to end Lights last
     //   

//        if( xf >= 0 && xf < gridCols && yf >= 0 && yf < gridRows )
   //         pLt0[ yf*gridCols + xf ] = ptColor;

        // next line segment
        P0.x = x0 = xf;
        P0.y = y0 = yf;
        // retard currPointIter = point moving back towards
        if (growState == 1)// growing. Stop at start point 0
        {
            if (currPointIter > 0) --currPointIter;
            else break;// growing from start point = pathX[0], pathY[0]
        }
        else if (currPointIter > 0) --currPointIter;
        else currPointIter = numPoints - 1;// keep going
        // new prevPos is end of line
        Pf.x = xf = pathX[currPointIter];
        Pf.y = yf = pathY[currPointIter];
        line = Pf - P0;
        steep = (line.y * line.y > line.x * line.x);
        dIter = get_dIter(line);
        stepLength = dIter.mag();

    }// end while
}

// simple version. No fade
void PointPlayer::draw2()const// for all segments
{
    if (!pLt0) return;
    if (!isPlaying) return;

    // the trail
    vec2f trailIter = pos, iterDir = uCurr;
    vec2f prevPos = pos0;// easy init
    uint8_t currPointIter = currPoint;// easy init
    // calculate uPrev per leg back. It is the new iterDir
    float trailDist = 0.0f;// go to fadeLength
    if (growState == -1) trailDist = growLength;// head start

    // draw line segments
    while (trailDist < fadeLength)
    {
        // find ends for line draw
        int x0 = pathX[currPointIter], y0 = pathY[currPointIter];
        int xf = trailIter.x, yf = trailIter.y;
        // is the remaining distance = fadeLength - trailDist < dist0f ?
        float remDist = fadeLength - trailDist;
        int distSq = (xf - x0) * (xf - x0) + (yf - y0) * (yf - y0);
        float dist0f = sqrtf(distSq);

        if (distSq > remDist * remDist)
        {
            // adjust x0, y0
            float frDist = remDist / dist0f;// fractional distance

            x0 = xf - (xf - x0) * frDist;
            y0 = yf - (yf - y0) * frDist;
            dist0f = remDist;// done
        }

        // validate endpoints are in the led grid
        if (x0 < 0) x0 = 0;
        else if (x0 >= gridCols) x0 = gridCols - 1;
        if (xf < 0) xf = 0;
        else if (xf >= gridCols) xf = gridCols - 1;

        if (y0 < 0) y0 = 0;
        else if (y0 >= gridRows) y0 = gridRows - 1;
        if (yf < 0) yf = 0;
        else if (yf >= gridRows) yf = gridRows - 1;
        // draw the line
        drawLine(x0, y0, xf, yf, ptColor);
        // increment trailDist
        trailDist += dist0f;
        if (trailDist >= fadeLength) break;

        // next segment
        trailIter.x = pathX[currPointIter];
        trailIter.y = pathY[currPointIter];
        // retard currPointIter = point moving back towards
        if (growState == 1)// growing. Stop at start point 0
        {
            if (currPointIter > 0) --currPointIter;
            else break;// growing from start point = pathX[0], pathY[0]
        }
        else if (currPointIter > 0) --currPointIter;
        else currPointIter = numPoints - 1;// keep going
    }
}

// static function. Implements Bresenham's algorithm
void PointPlayer::drawLine(int8_t x0, int8_t y0, int8_t x1, int8_t y1, Light color)const
{
    int8_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int8_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy, e2;

    while (true)
    {
        //  leds[XY(x0, y0)] = color; // Rasterize: set the pixel
        pLt0[x0 + y0 * gridCols] = color;// Rasterize: set the pixel
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/*
// raster + fade
void PointPlayer::draw3()const
{
    if( !pLt0 ) return;
    if( !isPlaying ) return;

    // the trail
    uint8_t currPointIter = currPoint;// easy init
    float trailDist = 0.0f;// go to fadeLength
    if( growState == -1 ) trailDist = growLength;// head start
    // fix ends at integer position
    int x0 = floor(pos.x), y0 = floor(pos.y);
    int xf = pathX[ currPointIter ], yf = pathY[ currPointIter ];

    while( trailDist < fadeLength )
    {
        // will write to end Lights last
        vec2f P0(x0,y0), Pf(xf,yf);// ends for here
        vec2f line = Pf - P0;
        bool steep = ( line.y*line.y > line.x*line.x );
        vec2f dIter = get_dIter( line );
        float stepLength = dIter.mag();// increment trailDist
        for( vec2f iter = P0 + dIter; ( Pf - iter ).dot( line ) >= 0.0f;  iter += dIter )// until iter passes Pf
        {
            trailDist += stepLength;
            if( trailDist >= fadeLength ) return;// done!

            int r = floor(iter.y), c = floor(iter.x);
            // bound check
            if( r < 0 )
            { if( line.y < 0.0f ) break; else continue; }// break if going up
            else if( r >= gridRows )
            { if( line.y > 0.0f ) break; else continue; }// break if going down
            if( c < 0 )
            { if( line.x < 0.0f ) break; else continue; }
            else if( c >= gridCols )
            { if( line.x > 0.0f ) break; else continue; }

            float U = trailDist/fadeLength;// fade factor

            if( steep )// write to left and right of the line
            {
                // weights for Light colors
                float fx = iter.x - c, rfx = 1.0f - fx;// fraction and remaining fraction
                rfx *= U;// apply fade factor
                fx = 1.0f - rfx;
                int n = r*gridCols + c;
                pLt0[n] = Light( rfx*ptColor.r + fx*pLt0[n].r, rfx*ptColor.g + fx*pLt0[n].g, rfx*ptColor.b + fx*pLt0[n].b );
                // 2nd to right since floor went to left
                if( ++c < gridCols )
                {   // apply other fraction
                    ++n;
                    pLt0[n] = Light( fx*ptColor.r + rfx*pLt0[n].r, fx*ptColor.g + rfx*pLt0[n].g, fx*ptColor.b + rfx*pLt0[n].b );
                }
            }
            else// write above and below
            {
                float fy = iter.y - r, rfy = 1.0f - fy;// fraction and remaining fraction
                rfy *= U;// apply fade factor
                fy = 1.0f - rfy;
                int n = r*gridCols + c;
                pLt0[n] = Light( rfy*ptColor.r + fy*pLt0[n].r, rfy*ptColor.g + fy*pLt0[n].g, rfy*ptColor.b + fy*pLt0[n].b );
                // 2nd below since floor went up
                if( ++r < gridRows )
                {   // apply other fraction
                    n += gridCols;
                    pLt0[n] = Light( fy*ptColor.r + rfy*pLt0[n].r, fy*ptColor.g + rfy*pLt0[n].g, fy*ptColor.b + rfy*pLt0[n].b );
                }
            }

         //   trailDist += stepLength;
         //   if( trailDist >= fadeLength ) return;// done!

        }// end for

        // will write to end Lights last
     //   if( x0 >= 0 && x0 < gridCols && y0 >= 0 && y0 < gridRows )
     //       pLt0[ y0*gridCols + x0 ] = ptColor;

        if( xf >= 0 && xf < gridCols && yf >= 0 && yf < gridRows )
            pLt0[ yf*gridCols + xf ] = ptColor;

        // next line segment
        x0 = xf; y0 = yf;
        // retard currPointIter = point moving back towards
        if( growState == 1 )// growing. Stop at start point 0
        {
            if( currPointIter > 0 ) --currPointIter;
            else break;// growing from start point = pathX[0], pathY[0]
        }
        else if( currPointIter > 0 ) --currPointIter;
        else currPointIter = numPoints - 1;// keep going
        // new prevPos is end of line
        xf = pathX[ currPointIter ];
        yf = pathY[ currPointIter ];
    }// end while
}

void PointPlayer::draw()const
{
    if( !pLt0 ) return;
    if( !isPlaying ) return;

    int r = pos.y, c = pos.x;
    // check bounds?
    if( r >= 0 && r < gridRows && c >= 0 && c < gridCols )
        pLt0[ r*gridCols + c ] = ptColor;

    // the trail
    vec2f trailIter = pos, iterDir = uCurr;
    float trailDist = 0.0f;// go to fadeLength
    bool firstLeg = true;
    while( trailDist < fadeLength )
    {
        trailDist += 0.5f;
        trailIter -= 0.5f*iterDir;
        if( firstLeg && uCurr.dot( trailIter - pos0 ) < 0.0f )// last point slightly over shot
        {
            firstLeg = false;
            trailIter = pos0;
            iterDir = uPrev;
        }

        int r = trailIter.y, c = trailIter.x;
        // check bounds?
        if( r >= 0 && r < gridRows && c >= 0 && c < gridCols )
        {

            float U = trailDist/fadeLength;
            float W = 1.0f - U;
            if( W <= 0.0f ) break;// fully faded
            int n = r*gridCols + c;
            float fr =  W*ptColor.r + U*pLt0[n].r;
            float fg =  W*ptColor.g + U*pLt0[n].g;
            float fb =  W*ptColor.b + U*pLt0[n].b;
            Light blendLt( fr, fg, fb );
            pLt0[n] = blendLt;
        }
    }
}
*/
