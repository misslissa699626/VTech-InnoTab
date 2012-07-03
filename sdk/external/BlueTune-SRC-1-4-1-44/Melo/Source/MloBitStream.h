/*****************************************************************
|
|    Melo - Bit Streams
|
|    Copyright 2004-2006 Axiomatic Systems LLC
|
|    This file is part of Melo (Melo AAC Decoder).
|
|    Unless you have obtained Melo under a difference license,
|    this version of Melo is Melo|GPL.
|    Melo|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Melo|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Melo|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
 ****************************************************************/
/** @file
 * Melo - Bit Streams
 */

#ifndef _MLO_BIT_STREAM_H_
#define _MLO_BIT_STREAM_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "MloConfig.h"
#include "MloTypes.h"
#include "MloResults.h"

/*----------------------------------------------------------------------
|       constants
+---------------------------------------------------------------------*/

/* error codes */
#define MLO_ERROR_NOT_ENOUGH_DATA        (MLO_ERROR_BASE_BITSTREAM - 0)
#define MLO_ERROR_CORRUPTED_BITSTREAM    (MLO_ERROR_BASE_BITSTREAM - 1)
#define MLO_ERROR_NOT_ENOUGH_FREE_BUFFER (MLO_ERROR_BASE_BITSTREAM - 2)

/*----------------------------------------------------------------------
|       types helpers
+---------------------------------------------------------------------*/
/* use long by default */
typedef unsigned int MLO_BitsWord;
#define MLO_WORD_BITS  32
#define MLO_WORD_BYTES 4

/*----------------------------------------------------------------------
|       types
+---------------------------------------------------------------------*/
typedef struct MLO_BitStream
{
    unsigned char* buffer;
    MLO_Size       buffer_size;
    MLO_Size       data_size;
    unsigned int   pos;
    MLO_BitsWord   cache;
    unsigned int   bits_cached;
} MLO_BitStream;

/*----------------------------------------------------------------------
|       prototypes
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    
MLO_Result MLO_BitStream_Construct(MLO_BitStream* bits, MLO_Size size);
MLO_Result MLO_BitStream_Destruct(MLO_BitStream* bits);
MLO_Result MLO_BitStream_SetData(MLO_BitStream*  bits, 
                                 const MLO_Byte* data, 
                                 MLO_Size        data_size);
MLO_Result MLO_BitStream_ByteAlign(MLO_BitStream* bits);
MLO_Result MLO_BitStream_Reset(MLO_BitStream* bits);
MLO_Size   MLO_BitStream_GetBitsLeft(MLO_BitStream* bits);

#ifdef __cplusplus
}
#endif /* __cplusplus */
    
/*----------------------------------------------------------------------
|       macros
+---------------------------------------------------------------------*/
#define MLO_BIT_MASK(_n) ((1<<(_n))-1)

/*
==============================================================================
Name: MLO_BitStream_ReadCache
Description:
   This is a private function, for internal use.
   Reads bytes to get cached bits.
Input parameters:
	- bits_ptr: Pointer on the BitStream structure
Returns: The cached bits
==============================================================================
*/

static inline MLO_BitsWord
MLO_BitStream_ReadCache (const MLO_BitStream* bits_ptr)
{
   unsigned int   pos = bits_ptr->pos;

#if MLO_WORD_BITS != 32
#error unsupported word size /* 64 and other word size not yet implemented */
#endif

   if (pos > bits_ptr->buffer_size - MLO_WORD_BYTES) return 0;

   {
      unsigned char *in = &bits_ptr->buffer[pos];
      return    (((MLO_BitsWord) in[0]) << 24)
              | (((MLO_BitsWord) in[1]) << 16)
              | (((MLO_BitsWord) in[2]) <<  8)
              | (((MLO_BitsWord) in[3])      );
   }
}

/*----------------------------------------------------------------------
|       MLO_BitStream_ReadBits
+---------------------------------------------------------------------*/
static inline unsigned int
MLO_BitStream_ReadBits(MLO_BitStream* bits, unsigned int n)
{
    MLO_BitsWord  result;
    if (bits->bits_cached >= n) {
        /* we have enough bits in the cache to satisfy the request */
        bits->bits_cached -= n;
        result = (bits->cache >> bits->bits_cached) & MLO_BIT_MASK(n);
    } else {
        /* not enough bits in the cache */
        MLO_BitsWord word;

        /* read the next word */
        word = MLO_BitStream_ReadCache (bits);
        bits->pos +=  MLO_WORD_BYTES;

        /* combine the new word and the cache, and update the state */
        {
            MLO_BitsWord cache = bits->cache & MLO_BIT_MASK(bits->bits_cached);
            n -= bits->bits_cached;
            bits->bits_cached = MLO_WORD_BITS - n;
            result = (word >> bits->bits_cached) | (cache << n);
            bits->cache = word;
        }
    }

    return result;
}

/*----------------------------------------------------------------------
|       MLO_BitStream_ReadBit
+---------------------------------------------------------------------*/
static inline unsigned int
MLO_BitStream_ReadBit(MLO_BitStream* bits)
{
    MLO_BitsWord result;
    if (bits->bits_cached == 0) {
        /* the cache is empty */

        /* read the next word into the cache */
        bits->cache = MLO_BitStream_ReadCache (bits);
        bits->pos +=  MLO_WORD_BYTES;
        bits->bits_cached = MLO_WORD_BITS - 1;

        /* return the first bit */
        result = bits->cache >> (MLO_WORD_BITS - 1);
    } else {
        /* get the bit from the cache */
        result = (bits->cache >> (--bits->bits_cached)) & 1;
    }
    return result;
}

/*----------------------------------------------------------------------
|       MLO_BitStream_PeekBits
+---------------------------------------------------------------------*/
static inline unsigned int
MLO_BitStream_PeekBits(const MLO_BitStream* bits, unsigned int n)
{
   /* we have enough bits in the cache to satisfy the request */
   if (bits->bits_cached >= n) {
      return (bits->cache >> (bits->bits_cached - n)) & MLO_BIT_MASK(n);
   } else {
      /* not enough bits in the cache */
      /* read the next word */
      MLO_BitsWord word = MLO_BitStream_ReadCache (bits);

      /* combine the new word and the cache, and update the state */
      MLO_BitsWord   cache = bits->cache & MLO_BIT_MASK(bits->bits_cached);
      n -= bits->bits_cached;
      return (word >> (MLO_WORD_BITS - n)) | (cache << n);
   }
}

/*----------------------------------------------------------------------
|       MLO_BitStream_PeekBit
+---------------------------------------------------------------------*/
static inline unsigned int
MLO_BitStream_PeekBit(const MLO_BitStream* bits)
{
   /* the cache is empty */
   if (bits->bits_cached == 0) {
      /* read the next word into the cache */
      MLO_BitsWord   cache = MLO_BitStream_ReadCache (bits);

      /* return the first bit */
      return cache >> (MLO_WORD_BITS - 1);
   } else {
      /* get the bit from the cache */
      return (bits->cache >> (bits->bits_cached-1)) & 1;
   }
}

/*----------------------------------------------------------------------
|       MLO_BitStream_SkipBits
+---------------------------------------------------------------------*/
static inline void
MLO_BitStream_SkipBits(MLO_BitStream* bits, unsigned int n)
{
   if (n <= bits->bits_cached) {
      bits->bits_cached -= n;
   } else {
      n -= bits->bits_cached;
      while (n >= MLO_WORD_BITS) {
         bits->pos +=  MLO_WORD_BYTES;
         n -= MLO_WORD_BITS;
      }
      if (n) {
         bits->cache = MLO_BitStream_ReadCache (bits);
         bits->bits_cached = MLO_WORD_BITS-n;
         bits->pos +=  MLO_WORD_BYTES;
      } else {
         bits->bits_cached = 0;
         bits->cache = 0;
      }
   }
}

/*----------------------------------------------------------------------
|       MLO_BitStream_SkipBit
+---------------------------------------------------------------------*/
static inline void
MLO_BitStream_SkipBit(MLO_BitStream* bits)
{
   if (bits->bits_cached == 0) {
      bits->cache = MLO_BitStream_ReadCache (bits);
      bits->pos +=  MLO_WORD_BYTES;
      bits->bits_cached = MLO_WORD_BITS - 1;
   } else {
      --bits->bits_cached;
   }
}


#endif /* _MLO_BIT_STREAM_H_ */
