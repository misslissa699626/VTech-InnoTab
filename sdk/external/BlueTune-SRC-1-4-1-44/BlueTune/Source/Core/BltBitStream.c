/*****************************************************************
|
|   BlueTune - Bit Streams
|
|   (c) 2002-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "BltConfig.h"
#include "BltTypes.h"
#include "BltBitStream.h"
#include "Atomix.h"

/*----------------------------------------------------------------------
|       BLT_BitStream_Construct
+---------------------------------------------------------------------*/
BLT_Result
BLT_BitStream_Construct(BLT_BitStream* bits, BLT_Size size)
{
    /* make the buffer size an integer number of words */
    size = BLT_WORD_BYTES*((size+BLT_WORD_BYTES-1)/BLT_WORD_BYTES);
    bits->buffer = (unsigned char*)ATX_AllocateMemory(size);
    bits->buffer_size = size;
    bits->data_size = 0;
    if (bits->buffer == NULL) return BLT_ERROR_OUT_OF_MEMORY;

    BLT_BitStream_Reset(bits);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       BLT_BitStream_Destruct
+---------------------------------------------------------------------*/
BLT_Result
BLT_BitStream_Destruct(BLT_BitStream* bits)
{
    BLT_BitStream_Reset(bits);
    if (bits->buffer != NULL) {
        ATX_FreeMemory(bits->buffer);
        bits->buffer = 0;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       BLT_BitStream_Reset
+---------------------------------------------------------------------*/
BLT_Result
BLT_BitStream_Reset(BLT_BitStream* bits)
{
    bits->pos         = 0;
    bits->bits_cached = 0;
    bits->cache       = 0;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       BLT_BitStream_GetBitsLeft
+---------------------------------------------------------------------*/
BLT_Size   
BLT_BitStream_GetBitsLeft(BLT_BitStream* bits)
{
    return 8*(bits->data_size-bits->pos)+bits->bits_cached;
}

/*----------------------------------------------------------------------
|       BLT_BitStream_SetData
+---------------------------------------------------------------------*/
BLT_Result 
BLT_BitStream_SetData(BLT_BitStream*       bits, 
                      const unsigned char* data, 
                      BLT_Size             data_size)
{
    if (data_size > bits->buffer_size) {
        return BLT_ERROR_OUT_OF_RANGE;
    }
    if (data == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    /* copy the data in the buffer */
    bits->data_size = data_size;
    ATX_CopyMemory(bits->buffer, data, data_size);

    /* fill the rest of the buffer with zeros */
    if (data_size != bits->buffer_size) {
        ATX_SetMemory(bits->buffer+data_size, 0, bits->buffer_size-data_size);
    }

    /* start from the beginning of the buffer */
    BLT_BitStream_Reset(bits);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       BLT_BitStream_ByteAlign
+---------------------------------------------------------------------*/
BLT_Result   
BLT_BitStream_ByteAlign(BLT_BitStream* bits)
{
    unsigned int to_flush = bits->bits_cached & 7;
    if (to_flush > 0) {
        BLT_BitStream_SkipBits(bits, to_flush);
    }
    return BLT_SUCCESS;
}
