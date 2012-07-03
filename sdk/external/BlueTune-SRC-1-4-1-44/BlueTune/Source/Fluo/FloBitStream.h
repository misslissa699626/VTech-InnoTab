/*****************************************************************
|
|   Fluo - Bit Stream
|
|   (c) 2002-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/** @file
 * Fluo - Bit Streams
 */

#ifndef _FLO_BIT_STREAM_H_
#define _FLO_BIT_STREAM_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "FloConfig.h"
#include "FloTypes.h"
#include "FloErrors.h"

/*----------------------------------------------------------------------
|       types helpers
+---------------------------------------------------------------------*/
/* use long by default */
typedef unsigned int FLO_BitsWord;
#define FLO_WORD_BITS  32
#define FLO_WORD_BYTES 4

/*----------------------------------------------------------------------
|       types
+---------------------------------------------------------------------*/
typedef struct FLO_BitStream
{
    const FLO_Byte* data;
    FLO_Size        data_size;
    unsigned int    pos;
    FLO_BitsWord    cache;
    unsigned int    bits_cached;
} FLO_BitStream;

/*----------------------------------------------------------------------
|       prototypes
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    
FLO_Result FLO_BitStream_SetData(FLO_BitStream*  bits, 
                                 const FLO_Byte* data, 
                                 FLO_Size        data_size);
FLO_Result FLO_BitStream_Reset(FLO_BitStream* bits);
FLO_Size   FLO_BitStream_GetBitsLeft(FLO_BitStream* bits);
FLO_Result FLO_BitStream_Rewind(FLO_BitStream* bits, unsigned int n);

#ifdef __cplusplus
}
#endif /* __cplusplus */
    
/*----------------------------------------------------------------------
|       macros
+---------------------------------------------------------------------*/
#define FLO_BIT_MASK(_n) ((1<<(_n))-1)

/*
==============================================================================
Name: FLO_BitStream_ReadCache
Description:
   This is a private function, for internal use.
   Reads bytes to get cached bits.
Input parameters:
	- bits_ptr: Pointer on the BitStream structure
Returns: The cached bits
==============================================================================
*/

static inline FLO_BitsWord
FLO_BitStream_ReadCache(const FLO_BitStream* bits_ptr)
{
   unsigned int   pos = bits_ptr->pos;

#if FLO_WORD_BITS != 32
#error unsupported word size /* 64 and other word size not yet implemented */
#endif

   if (pos > bits_ptr->data_size - FLO_WORD_BYTES) return 0;

   {
      const unsigned char *in = &bits_ptr->data[pos];
      return    (((FLO_BitsWord) in[0]) << 24)
              | (((FLO_BitsWord) in[1]) << 16)
              | (((FLO_BitsWord) in[2]) <<  8)
              | (((FLO_BitsWord) in[3])      );
   }
}

/*----------------------------------------------------------------------
|       FLO_BitStream_ReadBits
+---------------------------------------------------------------------*/
static inline unsigned int
FLO_BitStream_ReadBits(FLO_BitStream* bits, unsigned int n)
{
    FLO_BitsWord  result;
    if (bits->bits_cached >= n) {
        /* we have enough bits in the cache to satisfy the request */
        bits->bits_cached -= n;
        result = (bits->cache >> bits->bits_cached) & FLO_BIT_MASK(n);
    } else {
        /* not enough bits in the cache */
        FLO_BitsWord word;

        /* read the next word */
        word = FLO_BitStream_ReadCache(bits);
        bits->pos +=  FLO_WORD_BYTES;

        /* combine the new word and the cache, and update the state */
        {
            FLO_BitsWord cache = bits->cache & FLO_BIT_MASK(bits->bits_cached);
            n -= bits->bits_cached;
            bits->bits_cached = FLO_WORD_BITS - n;
            result = (word >> bits->bits_cached) | (cache << n);
            bits->cache = word;
        }
    }

    return result;
}

/*----------------------------------------------------------------------
|       FLO_BitStream_ReadBit
+---------------------------------------------------------------------*/
static inline unsigned int
FLO_BitStream_ReadBit(FLO_BitStream* bits)
{
    FLO_BitsWord   result;
    if (bits->bits_cached == 0) {
        /* the cache is empty */

        /* read the next word into the cache */
        bits->cache = FLO_BitStream_ReadCache (bits);
        bits->pos +=  FLO_WORD_BYTES;
        bits->bits_cached = FLO_WORD_BITS - 1;

        /* return the first bit */
        result = bits->cache >> (FLO_WORD_BITS - 1);
    } else {
        /* get the bit from the cache */
        result = (bits->cache >> (--bits->bits_cached)) & 1;
    }
    return result;
}

/*----------------------------------------------------------------------
|       FLO_BitStream_PeekBits
+---------------------------------------------------------------------*/
static inline unsigned int
FLO_BitStream_PeekBits(const FLO_BitStream* bits, unsigned int n)
{
   /* we have enough bits in the cache to satisfy the request */
   if (bits->bits_cached >= n) {
      return (bits->cache >> (bits->bits_cached - n)) & FLO_BIT_MASK(n);
   } else {
      /* not enough bits in the cache */
      /* read the next word */
      FLO_BitsWord word = FLO_BitStream_ReadCache(bits);

      /* combine the new word and the cache, and update the state */
      FLO_BitsWord   cache = bits->cache & FLO_BIT_MASK(bits->bits_cached);
      n -= bits->bits_cached;
      return (word >> (FLO_WORD_BITS - n)) | (cache << n);
   }
}

/*----------------------------------------------------------------------
|       FLO_BitStream_PeekBit
+---------------------------------------------------------------------*/
static inline unsigned int
FLO_BitStream_PeekBit(const FLO_BitStream* bits)
{
   /* the cache is empty */
   if (bits->bits_cached == 0) {
      /* read the next word into the cache */
      FLO_BitsWord cache = FLO_BitStream_ReadCache(bits);

      /* return the first bit */
      return cache >> (FLO_WORD_BITS - 1);
   } else {
      /* get the bit from the cache */
      return (bits->cache >> (bits->bits_cached-1)) & 1;
   }
}

/*----------------------------------------------------------------------
|       FLO_BitStream_SkipBits
+---------------------------------------------------------------------*/
static inline void
FLO_BitStream_SkipBits(FLO_BitStream* bits, unsigned int n)
{
   if (n <= bits->bits_cached) {
      bits->bits_cached -= n;
   } else {
      n -= bits->bits_cached;
      while (n >= FLO_WORD_BITS) {
         bits->pos +=  FLO_WORD_BYTES;
         n -= FLO_WORD_BITS;
      }
      if (n) {
         bits->cache = FLO_BitStream_ReadCache(bits);
         bits->bits_cached = FLO_WORD_BITS-n;
         bits->pos +=  FLO_WORD_BYTES;
      } else {
         bits->bits_cached = 0;
         bits->cache = 0;
      }
   }
}

/*----------------------------------------------------------------------
|       FLO_BitStream_SkipBit
+---------------------------------------------------------------------*/
static inline void
FLO_BitStream_SkipBit(FLO_BitStream* bits)
{
   if (bits->bits_cached == 0) {
      bits->cache = FLO_BitStream_ReadCache(bits);
      bits->pos +=  FLO_WORD_BYTES;
      bits->bits_cached = FLO_WORD_BITS - 1;
   } else {
      --bits->bits_cached;
   }
}

#endif /* _FLO_BIT_STREAM_H_ */
