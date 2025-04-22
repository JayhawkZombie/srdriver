#include "bitArray.h"

void bitArray::initClear(uint8_t &Char0, unsigned int CapBytes)// clears for fill via push()
{
    pByte = &Char0;
    capBytes = CapBytes;
    sizeBits = 0;// unsure if this will be usefull
    for (unsigned int k = 0; k < capBytes; ++k)
        *(pByte + k) = 0;// to prepare for data via push(bool)
}

void bitArray::init(uint8_t &Char0, unsigned int CapBytes)// for prepared data
{
    pByte = &Char0;
    capBytes = CapBytes;
    sizeBits = 8 * CapBytes;
}

// bool bitArray::loadBitsFromStream2(std::istream &is, unsigned int numVals)
// {
//     bool inVal;
//     for (unsigned int k = 0; k < numVals && k < 8 * capBytes; ++k)
//     {
//         is >> inVal;
//         push(inVal);
//     }

//     return true;
// }

// bool bitArray::loadBitsFromStream(std::istream &is)// as 8*capBytes characters = '0' or '1'
// {
//     unsigned char inBit = 0;//, inByte = 0;
//     unsigned char twoPow = 1;

//     uint8_t inByte = 0;
//     uint8_t *pIter = pByte;

//     for (unsigned int k = 0; k < 8 * capBytes; ++k)
//     {
//         is >> inBit;// '0' or '1'

//         inByte += twoPow * inBit;

//         if (twoPow < 128)
//         {
//             twoPow *= 2;
//         }
//         else// write and advance
//         {
//             *pIter = inByte;
//             ++pIter;
//             twoPow = 1;
//         }
//     }

//     sizeBits = 8 * capBytes;

//     return true;
// }

//   const uint8_t* pTgt = pByte + n/8;
 //   n %= 8;
 //   uint8_t mask = 1 << n;
    // bitwise and target byte with mask
  //  uint8_t result = *pTgt & mask;
 //   return result > 0;
 // from getDblBit
 //   uint8_t retVal = getBit( 2*n ) ? 1 : 0;
  //  retVal = retVal << 1;
  //  retVal += getBit( 2*n + 1 ) ? 1 : 0;
 //   return retVal;

bool bitArray::getBit(unsigned int n)const// return n th of 8*capBytes bits?
{
    const uint8_t *pTgt = pByte + n / 8;// target Byte
    uint8_t Byte = *pTgt >> n % 8;// target bit: 0 to 7
    return (Byte & 1) > 0;// return bool value
}

uint8_t bitArray::getDblBit(unsigned int n)const//
{
    const uint8_t *pTgt = pByte + n / 4;// target Byte
    uint8_t Byte = *pTgt >> 2 * (n % 4);// target bit: 0,2,4,6
    return (Byte & 3);// = 0,1,2 or 3
}

// working
uint8_t bitArray::getQuadBit(unsigned int n)const// reads from bits 4*n to 4*n + 3. returns 0 to 15
{
    uint8_t chkVal = 8 * getBit(4 * n) + 4 * getBit(4 * n + 1) + 2 * getBit(4 * n + 2) + getBit(4 * n + 3);

    //    uint8_t chkVal = 4*getDblBit( 4*n );// does not work
    //    chkVal += getDblBit( 4*n + 2 );

    unsigned XchkVal = chkVal;
    //    std::cout << "\n chkVal = " << XchkVal;
    return chkVal;// = 0 to 15
}

/*
uint8_t bitArray::getQuadBit( unsigned int n )const// reads from bits 4*n to 4*n + 3. returns 0 to 15
{
    const uint8_t* pTgt = pByte + n/2;// target Byte
    uint8_t Byte = *pTgt >> 4*(n%2);// target bit: 0,4    wrong
 //   uint8_t Byte = *pTgt >> 4*(n/4);// target bit: 0,4  more wrong

    unsigned int ByteX = ( Byte & 15 );
    std::cout << "\n QuadBit = " << ByteX;

    return ( Byte & 15 );// = 0 to 15
}
*/

void bitArray::setBit(unsigned int n, unsigned char binVal)const// changing value pointed to, not a member.
{
    uint8_t *pTgt = pByte + n / 8;
    n %= 8;
    uint8_t mask = 1 << n;

    if (binVal == 1)// write 1
    {
        // bitwise or with eg. 00000100 for n = 2
        *pTgt = *pTgt | mask;
    }
    else// write 0 to n th bit
    {
        // bitwise and with eg 11111011 for n = 2
        mask = 255 - mask;
        *pTgt = *pTgt & mask;
    }

    return;
}

// writes to bits 2*n and 2*n + 1
void bitArray::setDblBit(unsigned int n, uint8_t Val)const
{
    setBit(2 * n, Val / 2 > 0);// hi bit 1st
    setBit(2 * n + 1, Val % 2 > 0);// lo bit 2nd
}


// writes to bits 4*n to 4*n + 3
void bitArray::setQuadBit(unsigned int n, uint8_t Val)const
{
    bool bit[4];
    bit[0] = (Val >> 3 & 1) > 0;
    setBit(4 * n, (Val >> 3 & 1) > 0);// hi bit 1st
    bit[1] = (Val >> 2 & 1) > 0;
    setBit(4 * n + 1, (Val >> 2 & 1) > 0);
    bit[2] = (Val >> 1 & 1) > 0;
    setBit(4 * n + 2, (Val >> 1 & 1) > 0);
    bit[3] = (Val & 1) > 0;
    setBit(4 * n + 3, (Val & 1) > 0);

    uint8_t chkVal = 8 * bit[0] + 4 * bit[1] + 2 * bit[2] + bit[3];
    unsigned int XVal = Val, XchkVal = chkVal;
    //   std::cout << "\n inVal = " << XVal << " chkVal = " << XchkVal;

    //   setBit( n, (Val >> 3 & 1) > 0 );// hi bit 1st
     //  setBit( n + 1, (Val >> 2 & 1) > 0 );
     //  setBit( n + 2, (Val >> 1 & 1) > 0 );
     //  setBit( n + 3, (Val & 1) > 0 );
}

/*
void bitArray::setQuadBit( unsigned int n, uint8_t Val )const
{
    setBit( n, 0 );// hi bit 1st
 //   setBit( 4*n + 1, 1 );
 //   setBit( 4*n + 2, 0 );
 //   setBit( 4*n + 3, 1 );
}
*/

// copy from or write to entire block of capBytes Bytes
void bitArray::copyFrom(const unsigned char *pSrc)
{
    for (unsigned int k = 0; k < capBytes; ++k)
        *(pByte + k) = *(pSrc + k);
}

void bitArray::copyTo(unsigned char *pTgt)const
{
    for (unsigned int k = 0; k < capBytes; ++k)
        *(pTgt + k) = *(pByte + k);
}

void bitArray::view(unsigned int bitsPerRow)const
{
    // const uint8_t *pIter = pByte;
    // //    pIter += pByte;

    // unsigned int bitCount = 0;
    // uint8_t mask = 1;

    // unsigned int numBits = sizeBits;
    // while (numBits > 0)
    // {
    //     // write a bit to console
    //     uint8_t result = *pIter & mask;
    //     if (result > 0) std::cout << '1';
    //     else std::cout << '0';

    //     // cycle mask
    //     if (mask < 128) mask *= 2;
    //     //    if( mask < 128 ) mask << 1;// does not work to replace *= 2
    //     else
    //     {
    //         mask = 1;
    //         ++pIter;
    //     }

    //     // new line
    //     if ((++bitCount) % bitsPerRow == 0)
    //         std::cout << '\n';

    //     --numBits;
    // }
}

void bitArray::viewDbl(unsigned int twoBitsPerRow)const
{
    // for (unsigned int k = 0; k < bitSize(); k += 2)
    // {
    //     unsigned int val = getBit(k) ? 1 : 0;
    //     val = val << 1;
    //     val += getBit(k + 1) ? 1 : 0;

    //     if (k % (2 * twoBitsPerRow) == 0) std::cout << '\n';
    //     std::cout << val << ' ';

    // }
}

void bitArray::viewBytes()const
{
    // std::cout << '\n';
    // for (unsigned int k = 0; k < capBytes; ++k)
    // {
    //     unsigned int val = *(pByte + k);
    //     std::cout << val << '\n';
    // }
}

/*
void bitArray::viewDbl( unsigned int twoBitsPerRow )const
{
    const uint8_t* pIter = pByte;
//    pIter += pByte;

    unsigned int bitCount = 0;
    uint8_t mask = 3;

    unsigned int numBits = sizeBits;
    while( numBits > 0 )
    {
        // write a bit to console
        uint8_t result = *pIter & mask;
        if( result == 0 ) std::cout << '0';
        else if( result == 1 ) std::cout << '1';
        else if( result == 2 ) std::cout << '2';
        else if( result == 3 ) std::cout << '3';

        // cycle mask
        if( mask < 128 ) mask *= 4;
    //    if( mask < 128 ) mask << 1;// does not work to replace *= 2
        else
        {
            mask = 3;
            ++pIter;
        }

        // new line
        if( ( bitCount += 2 )%twoBitsPerRow == 0 )
            std::cout << '\n';

        numBits -= 2;
    }
}
*/
