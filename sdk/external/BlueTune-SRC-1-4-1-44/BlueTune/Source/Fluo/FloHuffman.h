/*****************************************************************
|
|   Fluo - Huffman Decoding
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _FLO_HUFFMAN_H_
#define _FLO_HUFFMAN_H_

/*-------------------------------------------------------------------------
|       includes
+-------------------------------------------------------------------------*/
#include "FloConfig.h"
#include "FloBitStream.h"
#include "FloTables.h"

/*-------------------------------------------------------------------------
|       types
+-------------------------------------------------------------------------*/
typedef struct {
    int          linbits;
    const short *lookup;
} FLO_HuffmanTable;
 
/*-------------------------------------------------------------------------
|       FLO_Huffman_DecodePair
+-------------------------------------------------------------------------*/
#define FLO_Huffman_DecodePair(bits, table, factor, sample, bits_left, inc)\
{                                                                       \
    const short *lookup = table->lookup;                                \
    short  value;                                                       \
    int    x,y;                                                         \
                                                                        \
    while ((value = *lookup++) < 0 ) {                                  \
        if (FLO_BitStream_ReadBit(bits)) lookup -= value;               \
        bits_left--;                                                    \
    }                                                                   \
    x = value >> 4;                                                     \
    y = value & 0x0F;                                                   \
    if (x) {                                                            \
        if (x == 15) {                                                  \
            x += FLO_BitStream_ReadBits(bits, table->linbits);          \
            bits_left -= table->linbits;                                \
        }                                                               \
        if (FLO_BitStream_ReadBit(bits)) {                              \
            *sample = -FLO_FC8_MUL(factor, FLO_Power_4_3[x]);           \
        } else {                                                        \
            *sample =  FLO_FC8_MUL(factor, FLO_Power_4_3[x]);           \
        }                                                               \
        bits_left--;                                                    \
    } else {                                                            \
        *sample = FLO_ZERO;                                             \
    }                                                                   \
    sample += inc;                                                      \
    if (y) {                                                            \
        if (y == 15) {                                                  \
            y += FLO_BitStream_ReadBits(bits, table->linbits);          \
            bits_left -= table->linbits;                                \
        }                                                               \
        if (FLO_BitStream_ReadBit(bits)) {                              \
            *sample = -FLO_FC8_MUL(factor, FLO_Power_4_3[y]);           \
        } else {                                                        \
            *sample =  FLO_FC8_MUL(factor, FLO_Power_4_3[y]);           \
        }                                                               \
        bits_left--;                                                    \
    } else {                                                            \
        *sample = FLO_ZERO;                                             \
    }                                                                   \
    sample += inc;                                                      \
}

/*-------------------------------------------------------------------------
|       FLO_Huffman_DecodeQuad
+-------------------------------------------------------------------------*/
#define FLO_Huffman_DecodeQuad(bits, table, quad, bits_left)            \
{                                                                       \
    const short *lookup = table->lookup;                                \
                                                                        \
    while ((quad = *lookup++) < 0 ) {                                   \
        if (FLO_BitStream_ReadBit(bits)) lookup -= quad;                \
        bits_left--;                                                    \
    }                                                                   \
}

/*-------------------------------------------------------------------------
|       prototypes
+-------------------------------------------------------------------------*/
extern FLO_HuffmanTable const FLO_HuffmanTables_Pair[32];
extern FLO_HuffmanTable const FLO_HuffmanTables_Quad[2];

#endif /* _FLO_HUFFMAN_H_ */






