#ifndef BITARRAY_H
#define BITARRAY_H

#include<stdint.h>// to write state to  console for viewing

// use an array of type unsigned char
class bitArray
{
    private:
    uint8_t* pByte = nullptr;
    unsigned int capBytes = 0;
    unsigned int sizeBits = 0;// role ??

    public:
    void initClear( uint8_t& Char0, unsigned int CapBytes );
    void init( uint8_t& Char0, unsigned int CapBytes );
    bitArray( uint8_t& Char0, unsigned int CapBytes ) { init( Char0, CapBytes ); }


    bitArray( const bitArray& ) = delete;// no copy
    bitArray& operator = ( const bitArray& ) = delete;// no assignment

    unsigned int ByteCapacity()const { return capBytes; }
    unsigned int bitCapacity()const { return 8*capBytes; }
    unsigned int bitSize()const { return sizeBits; }

    // bool loadBitsFromStream( std::istream& is );// as 8*capBytes characters = '0' or '1'
    // bool loadBitsFromStream2( std::istream& is, unsigned int numVals );// as 8*capBytes characters = '0' or '1'

    // copy from or write to entire block of capBytes Bytes
    void copyFrom( const unsigned char* pSrc );
    void copyTo( unsigned char* pTgt )const;

    bool getBit( unsigned int n )const;// return n th of 8*capBytes bits?
    void setBit( unsigned int n, unsigned char binVal )const;// changing value pointed to, not a member.
    void Clear(){ sizeBits = 0; }
    void reSize( unsigned int newSz ){ sizeBits = newSz; }
    void pop(){ if( sizeBits > 0 ) --sizeBits; }
    void push( bool bitVal )// increments sizeBits
    {
        setBit( sizeBits, bitVal > 0 );
        ++sizeBits;// no re allocation when capacity = 8*capBytes is reached
    }

    // void view( unsigned int bitsPerRow = 8 )const;
    // void viewDbl( unsigned int twoBitsPerRow = 8 )const;

    // void viewBytes()const;

    bitArray(){}
    ~bitArray(){}

    protected:


};

#endif // BITARRAY_H
