/*************************************************************************
* Name:        lz.c
* Author:      Marcus Geelnard
* Description: LZ77 coder/decoder implementation.
* Reentrant:   Yes
*
* The LZ77 compression scheme is a substitutional compression scheme
* proposed by Abraham Lempel and Jakob Ziv in 1977. It is very simple in
* its design, and uses no fancy bit level compression.
*
* This is my first attempt at an implementation of a LZ77 code/decoder.
*
* The principle of the LZ77 compression algorithm is to store repeated
* occurrences of strings as references to previous occurrences of the same
* string. The point is that the reference consumes less space than the
* string itself, provided that the string is long enough (in this
* implementation, the string has to be at least 4 bytes long, since the
* minimum coded reference is 3 bytes long). Also note that the term
* "string" refers to any kind of byte sequence (it does not have to be
* an ASCII string, for instance).
*
* The coder uses a brute force approach to finding string matches in the
* history buffer (or "sliding window", if you wish), which is very, very
* slow. I recon the complexity is somewhere between O(n^2) and O(n^3),
* depending on the input data.
*
* There is also a faster implementation that uses a large working buffer
* in which a "jump table" is stored, which is used to quickly find
* possible string matches (see the source code for LZ_CompressFast() for
* more information). The faster method is an order of magnitude faster,
* but still quite slow compared to other compression methods.
*
* The upside is that decompression is very fast, and the compression ratio
* is often very good.
*
* The reference to a string is coded as a (length,offset) pair, where the
* length indicates the length of the string, and the offset gives the
* offset from the current data position. To distinguish between string
* references and literal strings (uncompressed bytes), a string reference
* is preceded by a marker byte, which is chosen as the least common byte
* symbol in the input data stream (this marker byte is stored in the
* output stream as the first byte).
*
* Occurrences of the marker byte in the stream are encoded as the marker
* byte followed by a zero byte, which means that occurrences of the marker
* byte have to be coded with two bytes.
*
* The lengths and offsets are coded in a variable length fashion, allowing
* values of any magnitude (up to 4294967295 in this implementation).
*
* With this compression scheme, the worst case compression result is
* (257/256)*insize + 1.
*
*-------------------------------------------------------------------------
* Copyright (c) 2003-2006 Marcus Geelnard
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would
*    be appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not
*    be misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source
*    distribution.
*
* Marcus Geelnard
* marcus.geelnard at home.se
*************************************************************************/


/*************************************************************************
* Constants used for LZ77 coding
*************************************************************************/

/* Maximum offset (can be any size < 2^31). Lower values give faster
   compression, while higher values gives better compression. The default
   value of 100000 is quite high. Experiment to see what works best for
   you. */
#define LZ_MAX_OFFSET 100000



/*************************************************************************
*                           INTERNAL FUNCTIONS                           *
*************************************************************************/


/*************************************************************************
* _LZ_StringCompare() - Return maximum length string match.
*************************************************************************/

static unsigned int _LZ_StringCompare( const unsigned char * str1,
  const unsigned char * str2, unsigned int minlen, unsigned int maxlen )
{
    unsigned int len;

    for( len = minlen; (len < maxlen) && (str1[len] == str2[len]); ++ len );

    return len;
}


/*************************************************************************
* _LZ_WriteVarSize() - Write unsigned integer with variable number of
* bytes depending on value.
*************************************************************************/

static int _LZ_WriteVarSize( unsigned int x, unsigned char * buf )
{
    unsigned int y;
    int num_bytes, i, b;

    /* Determine number of bytes needed to store the number x */
    y = x >> 3;
    for( num_bytes = 5; num_bytes >= 2; -- num_bytes )
    {
        if( y & 0xfe000000 ) break;
        y <<= 7;
    }

    /* Write all bytes, seven bits in each, with 8:th bit set for all */
    /* but the last byte. */
    for( i = num_bytes-1; i >= 0; -- i )
    {
        b = (x >> (i*7)) & 0x0000007f;
        if( i > 0 )
        {
            b |= 0x00000080;
        }
        *buf ++ = (unsigned char) b;
    }

    /* Return number of bytes written */
    return num_bytes;
}


/*************************************************************************
* _LZ_ReadVarSize() - Read unsigned integer with variable number of
* bytes depending on value.
*************************************************************************/

static int _LZ_ReadVarSize( unsigned int * x, const unsigned char * buf )
{
    unsigned int y, b, num_bytes;

    /* Read complete value (stop when byte contains zero in 8:th bit) */
    y = 0;
    num_bytes = 0;
    do
    {
        b = (unsigned int) (*buf ++);
        y = (y << 7) | (b & 0x0000007f);
        ++ num_bytes;
    }
    while( b & 0x00000080 );

    /* Store value in x */
    *x = y;

    /* Return number of bytes read */
    return num_bytes;
}



/*************************************************************************
*                            PUBLIC FUNCTIONS                            *
*************************************************************************/


/*************************************************************************
* LZ_Compress() - Compress a block of data using an LZ77 coder.
*  in     - Input (uncompressed) buffer.
*  out    - Output (compressed) buffer. This buffer must be 0.4% larger
*           than the input buffer, plus one byte.
*  insize - Number of input bytes.
* The function returns the size of the compressed data.
*************************************************************************/

int LZ_Compress( const unsigned char *in, unsigned char *out, unsigned int insize )
{
    unsigned char marker, symbol;
    unsigned int  inpos, outpos, bytesleft, i;
    unsigned int  maxoffset, offset, bestoffset;
    unsigned int  maxlength, length, bestlength;
    unsigned int  histogram[ 256 ];
    const unsigned char *ptr1, *ptr2;

    /* Do we have anything to compress? */
    if( insize < 1 )
    {
        return 0;
    }

    /* Create histogram */
    for( i = 0; i < 256; ++ i )
    {
        histogram[ i ] = 0;
    }
    for( i = 0; i < insize; ++ i )
    {
        ++ histogram[ in[ i ] ];
    }

    /* Find the least common byte, and use it as the marker symbol */
    marker = 0;
    for( i = 1; i < 256; ++ i )
    {
        if( histogram[ i ] < histogram[ marker ] )
        {
            marker = (unsigned char) i;
        }
    }

    /* Remember the marker symbol for the decoder */
    out[ 0 ] = marker;

    /* Start of compression */
    inpos = 0;
    outpos = 1;

    /* Main compression loop */
    bytesleft = insize;
    do
    {
        /* Determine most distant position */
        if( inpos > LZ_MAX_OFFSET ) maxoffset = LZ_MAX_OFFSET;
        else                        maxoffset = inpos;

        /* Get pointer to current position */
        ptr1 = &in[ inpos ];

        /* Search history window for maximum length string match */
        bestlength = 3;
        bestoffset = 0;
        for( offset = 3; offset <= maxoffset; ++ offset )
        {
            /* Get pointer to candidate string */
            ptr2 = &ptr1[ -(int)offset ];

            /* Quickly determine if this is a candidate (for speed) */
            if( (ptr1[ 0 ] == ptr2[ 0 ]) &&
                (ptr1[ bestlength ] == ptr2[ bestlength ]) )
            {
                /* Determine maximum length for this offset */
                maxlength = (bytesleft < offset ? bytesleft : offset);

                /* Count maximum length match at this offset */
                length = _LZ_StringCompare( ptr1, ptr2, 0, maxlength );

                /* Better match than any previous match? */
                if( length > bestlength )
                {
                    bestlength = length;
                    bestoffset = offset;
                }
            }
        }

        /* Was there a good enough match? */
        if( (bestlength >= 8) ||
            ((bestlength == 4) && (bestoffset <= 0x0000007f)) ||
            ((bestlength == 5) && (bestoffset <= 0x00003fff)) ||
            ((bestlength == 6) && (bestoffset <= 0x001fffff)) ||
            ((bestlength == 7) && (bestoffset <= 0x0fffffff)) )
        {
            out[ outpos ++ ] = (unsigned char) marker;
            outpos += _LZ_WriteVarSize( bestlength, &out[ outpos ] );
            outpos += _LZ_WriteVarSize( bestoffset, &out[ outpos ] );
            inpos += bestlength;
            bytesleft -= bestlength;
        }
        else
        {
            /* Output single byte (or two bytes if marker byte) */
            symbol = in[ inpos ++ ];
            out[ outpos ++ ] = symbol;
            if( symbol == marker )
            {
                out[ outpos ++ ] = 0;
            }
            -- bytesleft;
        }
    }
    while( bytesleft > 3 );

    /* Dump remaining bytes, if any */
    while( inpos < insize )
    {
        if( in[ inpos ] == marker )
        {
            out[ outpos ++ ] = marker;
            out[ outpos ++ ] = 0;
        }
        else
        {
            out[ outpos ++ ] = in[ inpos ];
        }
        ++ inpos;
    }

    return outpos;
}


/*************************************************************************
* LZ_Uncompress() - Uncompress a block of data using an LZ77 decoder.
*  in      - Input (compressed) buffer.
*  out     - Output (uncompressed) buffer. This buffer must be large
*            enough to hold the uncompressed data.
*  insize  - Number of input bytes.
*************************************************************************/

int LZ_Uncompress( const unsigned char *in, unsigned char *out, unsigned int insize )
{
    unsigned char marker, symbol;
    unsigned int  i, inpos, outpos, length, offset;

    /* Do we have anything to uncompress? */
    if( insize < 1 )
    {
        return 0;
    }

    /* Get marker symbol from input stream */
    marker = in[ 0 ];
    inpos = 1;

    /* Main decompression loop */
    outpos = 0;
    do
    {
        symbol = in[ inpos ++ ];
        if( symbol == marker )
        {
            /* We had a marker byte */
            if( in[ inpos ] == 0 )
            {
                /* It was a single occurrence of the marker byte */
                out[ outpos ++ ] = marker;
                ++ inpos;
            }
            else
            {
                /* Extract true length and offset */
                inpos += _LZ_ReadVarSize( &length, &in[ inpos ] );
                inpos += _LZ_ReadVarSize( &offset, &in[ inpos ] );

                /* Copy corresponding data from history window */
                for( i = 0; i < length; ++ i )
                {
                    out[ outpos ] = out[ outpos - offset ];
                    ++ outpos;
                }
            }
        }
        else
        {
            /* No marker, plain copy */
            out[ outpos ++ ] = symbol;
        }
    }
    while( inpos < insize );

    return outpos;
}
