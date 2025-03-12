#ifndef LIGHT_H
#define LIGHT_H

#include<stdint.h>

class Light
{
    public:
    uint8_t r = 1, g = 2, b = 3;

    void init( uint8_t Rd, uint8_t Gn, uint8_t Bu )
    { r = Rd; g = Gn; b = Bu;  }
    Light( uint8_t Rd, uint8_t Gn, uint8_t Bu ){ init( Rd, Gn, Bu ); }
    Light(){}

    bool operator == ( const Light& Lt ) const
    {
        if( r != Lt.r || g != Lt.g || b != Lt.b ) return false;
        return true;
    }
};

#endif // LIGHT_H
