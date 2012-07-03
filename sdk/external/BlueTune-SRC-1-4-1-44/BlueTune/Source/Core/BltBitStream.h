/*****************************************************************
|
|   BlueTune - Bit Streams
|
|   (c) 2002-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BLT_BitStream objects
 */

#ifndef _BLT_BIT_STREAM_H_
#define _BLT_BIT_STREAM_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "BltConfig.h"
#include "BltTypes.h"
#include "BltErrors.h"

/*----------------------------------------------------------------------
|       types helpers
+---------------------------------------------------------------------*/
/* use long by default */
typedef BLT_UInt32 BLT_BitsWord;
#define BLT_WORD_BITS  32
#define BLT_WORD_BYTES 4

/*----------------------------------------------------------------------
|       types
+---------------------------------------------------------------------*/
typedef struct BLT_BitStream
{
    unsigned char* buffer;
    BLT_Size       buffer_size;
    BLT_Size       data_size;
    unsigned int   pos;
    BLT_BitsWord   cache;
    unsigned int   bits_cached;
} BLT_BitStream;

/*----------------------------------------------------------------------
|       prototypes
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    
BLT_Result BLT_BitStream_Construct(BLT_BitStream* bits, BLT_Size size);
BLT_Result BLT_BitStream_Destruct(BLT_BitStream* bits);
BLT_Result BLT_BitStream_SetData(BLT_BitStream*       bits, 
                                 const unsigned char* data, 
                                 BLT_Size             data_size);
BLT_Result BLT_BitStream_ByteAlign(BLT_BitStream* bits);
BLT_Result BLT_BitStream_Reset(BLT_BitStream* bits);
BLT_Size   BLT_BitStream_GetBitsLeft(BLT_BitStream* bits);

#ifdef __cplusplus
}
#endif /* __cplusplus */
    
/*----------------------------------------------------------------------
|       macros
+---------------------------------------------------------------------*/
#define BLT_BIT_MASK(_n) ((1<<(_n))-1)

/*
==============================================================================
Name: BLT_BitStream_ReadCache
Description:
   This is a private function, for internal use.
   Reads bytes to get cached bits.
Input parameters:
	- bits_ptr: Pointer on the BitStream structure
Returns: The cached bits
==============================================================================
*/

static inline BLT_BitsWord
BLT_BitStream_ReadCache (const BLT_BitStream* bits_ptr)
{
   unsigned int   pos = bits_ptr->pos;

#if BLT_WORD_BITS != 32
#error unsupported word size /* 64 and other word size not yet implemented */
#endif

   if (pos > bits_ptr->buffer_size - BLT_WORD_BYTES) return 0;

   {
      unsigned char *in = &bits_ptr->buffer[pos];
      return    (((BLT_BitsWord) in[0]) << 24)
              | (((BLT_BitsWord) in[1]) << 16)
              | (((BLT_BitsWord) in[2]) <<  8)
              | (((BLT_BitsWord) in[3])      );
   }
}

/*----------------------------------------------------------------------
|       BLT_BitStream_ReadBits
+---------------------------------------------------------------------*/
static inline unsigned int
BLT_BitStream_ReadBits(BLT_BitStream* bits, unsigned int n)
{
    BLT_BitsWord  result;
    if (bits->bits_cached >= n) {
        /* we have enough bits in the cache to satisfy the request */
        bits->bits_cached -= n;
        result = (bits->cache >> bits->bits_cached) & BLT_BIT_MASK(n);
    } else {
        /* not enough bits in the cache */
        BLT_BitsWord word;

        /* read the next word */
        word = BLT_BitStream_ReadCache (bits);
        bits->pos +=  BLT_WORD_BYTES;

        /* combine the new word and the cache, and update the state */
        {
            BLT_BitsWord cache = bits->cache & BLT_BIT_MASK(bits->bits_cached);
            n -= bits->bits_cached;
            bits->bits_cached = BLT_WORD_BITS - n;
            result = (word >> bits->bits_cached) | (cache << n);
            bits->cache = word;
        }
    }

    return result;
}

/*----------------------------------------------------------------------
|       BLT_BitStream_ReadBit
+---------------------------------------------------------------------*/
static inline unsigned int
BLT_BitStream_ReadBit(BLT_BitStream* bits)
{
    BLT_BitsWord result;
    if (bits->bits_cached == 0) {
        /* the cache is empty */

        /* read the next word into the cache */
        bits->cache = BLT_BitStream_ReadCache (bits);
        bits->pos +=  BLT_WORD_BYTES;
        bits->bits_cached = BLT_WORD_BITS - 1;

        /* return the first bit */
        result = bits->cache >> (BLT_WORD_BITS - 1);
    } else {
        /* get the bit from the cache */
        result = (bits->cache >> (--bits->bits_cached)) & 1;
    }
    return result;
}

/*----------------------------------------------------------------------
|       BLT_BitStream_PeekBits
+---------------------------------------------------------------------*/
static inline unsigned int
BLT_BitStream_PeekBits(const BLT_BitStream* bits, unsigned int n)
{
   /* we have enough bits in the cache to satisfy the request */
   if (bits->bits_cached >= n) {
      return (bits->cache >> (bits->bits_cached - n)) & BLT_BIT_MASK(n);
   } else {
      /* not enough bits in the cache */
      /* read the next word */
      BLT_BitsWord word = BLT_BitStream_ReadCache (bits);

      /* combine the new word and the cache, and update the state */
      BLT_BitsWord   cache = bits->cache & BLT_BIT_MASK(bits->bits_cached);
      n -= bits->bits_cached;
      return (word >> (BLT_WORD_BITS - n)) | (cache << n);
   }
}

/*----------------------------------------------------------------------
|       BLT_BitStream_PeekBit
+---------------------------------------------------------------------*/
static inline unsigned int
BLT_BitStream_PeekBit(const BLT_BitStream* bits)
{
   /* the cache is empty */
   if (bits->bits_cached == 0) {
      /* read the next word into the cache */
      BLT_BitsWord   cache = BLT_BitStream_ReadCache (bits);

      /* return the first bit */
      return cache >> (BLT_WORD_BITS - 1);
   } else {
      /* get the bit from the cache */
      return (bits->cache >> (bits->bits_cached-1)) & 1;
   }
}

/*----------------------------------------------------------------------
|       BLT_BitStream_SkipBits
+---------------------------------------------------------------------*/
static inline void
BLT_BitStream_SkipBits(BLT_BitStream* bits, unsigned int n)
{
   if (n <= bits->bits_cached) {
      bits->bits_cached -= n;
   } else {
      n -= bits->bits_cached;
      while (n >= BLT_WORD_BITS) {
         bits->pos +=  BLT_WORD_BYTES;
         n -= BLT_WORD_BITS;
      }
      if (n) {
         bits->cache = BLT_BitStream_ReadCache (bits);
         bits->bits_cached = BLT_WORD_BITS-n;
         bits->pos +=  BLT_WORD_BYTES;
      } else {
         bits->bits_cached = 0;
         bits->cache = 0;
      }
   }
}

/*----------------------------------------------------------------------
|       BLT_BitStream_SkipBit
+---------------------------------------------------------------------*/
static inline void
BLT_BitStream_SkipBit(BLT_BitStream* bits)
{
   if (bits->bits_cached == 0) {
      bits->cache = BLT_BitStream_ReadCache (bits);
      bits->pos +=  BLT_WORD_BYTES;
      bits->bits_cached = BLT_WORD_BITS - 1;
   } else {
      --bits->bits_cached;
   }
}


#endif /* _BLT_BIT_STREAM_H_ */
