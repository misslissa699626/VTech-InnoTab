/*****************************************************************
|
|      File: BitStreamTest.c
|
|      Melo - Unit test for the BitStream library
|
|      (c) 2004 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "MloBitStream.h"

/*----------------------------------------------------------------------
|       constants
+---------------------------------------------------------------------*/
#define MAX_REPEATS 5000

/*----------------------------------------------------------------------
|       test1
+---------------------------------------------------------------------*/
static int
test1(MLO_BitStream* bits1, MLO_BitStream* bits2)
{
    unsigned int bit_count;
    unsigned int bits_to_read;
    unsigned int offset = 0;

    /* -------------------- test ReadBits and PeekBits ------------------------------- */
    /* reset the bit stream */
    MLO_BitStream_Reset(bits1);
    MLO_BitStream_Reset(bits2);
    bit_count = 0;
    bits_to_read = MLO_BITSTREAM_BUFFER_SIZE*16*8;

    /* read the data and check */
    while (bits_to_read > 0) {
        MLO_BitsWord read;
        MLO_BitsWord peek;
        MLO_BitsWord expected;
        

        /* pick how many beet to work on */
        unsigned int n = 1+ ((rand() >> 7)%32);
        if (n > bits_to_read) n = bits_to_read;

        /* read n bits */
        read = MLO_BitStream_ReadBits(bits1, n);

        /* peek n bits and skip */
        peek = MLO_BitStream_PeekBits(bits2, n);
        MLO_BitStream_SkipBits(bits2, n);

        /* check the value */
        {
            unsigned int p;
            expected = 0;
            for (p=0; p<n; p++) {
                unsigned int bit = bits1->buffer[((bit_count+p)/8)%MLO_BITSTREAM_BUFFER_SIZE] & (1 << (7-((bit_count+p)%8)));
                expected = (expected << 1) | (bit == 0 ? 0 : 1);
            }

            if (expected != read || expected != peek) {
                fprintf(stderr, "ERROR: expected %x, got [read=%x, peek=%x] [%d bits at offset %d (%d.%d)]\n", expected, read, peek, n, bit_count, bit_count/8, bit_count%8);
                for (offset=bit_count/8; offset<bit_count/8+4; offset++) {
                    fprintf(stderr, "%02x ", bits1->buffer[offset%MLO_BITSTREAM_BUFFER_SIZE]);
                }
                return -1;
            }
        }

        bit_count += n;
        bits_to_read -= n;
    }

    return 0;
}

/*----------------------------------------------------------------------
|       test2
+---------------------------------------------------------------------*/
static int
test2(MLO_BitStream* bits1, MLO_BitStream* bits2)
{
    unsigned int bit_count;
    unsigned int bits_to_read;
    unsigned int offset = 0;

    /* -------------------- test SkipBits, SkipBit, PeekBit and ReadBit ------------------------------- */

    /* reset the bit stream */
    MLO_BitStream_Reset(bits1);
    MLO_BitStream_Reset(bits2);
    bit_count = 0;
    bits_to_read = MLO_BITSTREAM_BUFFER_SIZE*16*8;

    /* read the data and check */
    while (bits_to_read > 0) {
        unsigned int read;
        unsigned int peek;
        unsigned int expected;
        unsigned int n;

        n = 1+ ((rand() >> 7)%128);
        if (n+1 > bits_to_read) n = bits_to_read-1;
        if (bits_to_read == 0) return 0;

        /* skip n bits in one call */
        MLO_BitStream_SkipBits(bits1, n);

        /* skip n times one bit */
        {
            unsigned int i;
            for (i=0; i<n; i++) {
                MLO_BitStream_SkipBit(bits2);
            }
        }
        bit_count += n;

        /* read one bit and check the value */
        read = MLO_BitStream_ReadBit(bits1);
        peek = MLO_BitStream_PeekBit(bits2);
        MLO_BitStream_SkipBit(bits2);
        expected = (bits1->buffer[((bit_count)/8)%MLO_BITSTREAM_BUFFER_SIZE] & (1 << (7-((bit_count)%8)))) == 0 ? 0 : 1;

        if (expected != read || expected != peek) {
            fprintf(stderr, "ERROR: expected %d, got [read=%d, peek=%d] [offset (%d.%d)]\n", expected, read, peek, bit_count/8, bit_count%8);
            for (offset=bit_count/8; offset<bit_count/8+4; offset++) {
                fprintf(stderr, "%02x ", bits1->buffer[offset%MLO_BITSTREAM_BUFFER_SIZE]);
            }
            return -1;
        }

        bit_count++;
        bits_to_read -= (n+1);
    }
    

    return 0;
}

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    unsigned int   repeat;
    MLO_BitStream  bits1;
    MLO_BitStream  bits2;

    /* allocate a bit stream */
    MLO_BitStream_AllocateBuffer(&bits1);
    MLO_BitStream_AllocateBuffer(&bits2);

    /* test loop */
    for (repeat=0; repeat<MAX_REPEATS; repeat++) {
        unsigned int offset;

        /* fill the buffer with random data */
        for (offset=0; offset<MLO_BITSTREAM_BUFFER_SIZE; offset++) {
            bits1.buffer[offset] = bits2.buffer[offset] = (rand() >> 7) & 0xFF;
        }

        if (test1(&bits1, &bits2) != 0) return 1;
        if (test2(&bits1, &bits2) != 0) return 1;
    }

    /* destroy the bit stream */
    MLO_BitStream_FreeBuffer(&bits1);
    MLO_BitStream_FreeBuffer(&bits2);

    return 0;
}
