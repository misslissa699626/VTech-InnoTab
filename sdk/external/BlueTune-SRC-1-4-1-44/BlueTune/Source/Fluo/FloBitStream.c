/*****************************************************************
|
|   Fluo - Bit Stream
|
|   (c) 2002-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       For efficiency reasons, this bitstream library only handles
|       data buffers that are a power of 2 in size
+---------------------------------------------------------------------*/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "FloConfig.h"
#include "FloTypes.h"
#include "FloBitStream.h"
#include "FloFrame.h"
#include "FloUtils.h"

#if (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN)

/*----------------------------------------------------------------------
|       FLO_BitStream_Reset
+---------------------------------------------------------------------*/
FLO_Result
FLO_BitStream_Reset(FLO_BitStream* bits)
{
    bits->pos         = 0;
    bits->bits_cached = 0;
    bits->cache       = 0;

    return FLO_SUCCESS;
}

/*----------------------------------------------------------------------
|       FLO_BitStream_GetBitsLeft
+---------------------------------------------------------------------*/
FLO_Size   
FLO_BitStream_GetBitsLeft(FLO_BitStream* bits)
{
    return 8*(bits->data_size-bits->pos)+bits->bits_cached;
}

/*----------------------------------------------------------------------
|       FLO_BitStream_Rewind
+---------------------------------------------------------------------*/
FLO_Result
FLO_BitStream_Rewind(FLO_BitStream* bits, unsigned int n)
{
    /* check that we don't rewind more than what we have */
    if (n > FLO_BitStream_GetBitsLeft(bits)) return FLO_ERROR_INVALID_PARAMETERS;
    
    /* rewind the cached bits first */
    {
        unsigned int can_rewind = FLO_WORD_BITS-bits->bits_cached;
        if (n <= can_rewind) {
            bits->bits_cached += n;
            return FLO_SUCCESS;
        }
        n -= can_rewind;
    }
    bits->bits_cached = 0;
    bits->pos -= FLO_WORD_BYTES;
    
    /* rewind remaining bits by chunks of FLO_WORD_BITS */
    while (n>FLO_WORD_BITS) {
        bits->pos -= FLO_WORD_BYTES;
        n -= FLO_WORD_BITS;
    }
    
    /* rewind whatever is left */
    if (n) {
        bits->pos -= FLO_WORD_BYTES;
        FLO_BitStream_SkipBits(bits, FLO_WORD_BITS-n);
    }
    
    return FLO_SUCCESS;
}

/*----------------------------------------------------------------------
|       FLO_BitStream_SetData
+---------------------------------------------------------------------*/
FLO_Result 
FLO_BitStream_SetData(FLO_BitStream*  bits, 
                      const FLO_Byte* data, 
                      FLO_Size        data_size)
{
    FLO_BitStream_Reset(bits);
    bits->data      = data;
    bits->data_size = data_size;

    return FLO_SUCCESS;
}

#endif /* FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN */
