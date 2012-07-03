/*****************************************************************
|
|    Melo - ADTS Parser
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

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "MloSamplingFreq.h"
#include "MloConfig.h"
#include "MloDebug.h"
#include "MloTypes.h"
#include "MloBitStream.h"
#include "MloFrame.h"
#include "MloAdtsParser.h"
#include "MloUtils.h"

#if 0
/*----------------------------------------------------------------------
|       types
+---------------------------------------------------------------------*/
struct MLO_AdtsParser {
    MLO_BitStream  bits;
    MLO_Cardinal   frame_count;
};

typedef struct {
    /* fixed part */
    unsigned int id;
    unsigned int protection_absent;
    unsigned int profile_object_type;
    unsigned int sampling_frequency_index;
    unsigned int channel_configuration;
    
    /* variable part */
    unsigned int frame_length;
    unsigned int raw_data_blocks;
} MLO_AdtsHeader;

/*----------------------------------------------------------------------
|       constants
+---------------------------------------------------------------------*/
#define MLO_ADTS_HEADER_SIZE 7

#define MLO_ADTS_SYNC_MASK     0xFFF6 /* 12 sync bits plus 2 layer bits */
#define MLO_ADTS_SYNC_PATTERN  0xFFF0 /* 12 sync bits=1 layer=0         */

/*----------------------------------------------------------------------+
|        MLO_AdtsHeader_MatchFixed
|
|    Check that two fixed headers are the same
|
+----------------------------------------------------------------------*/
static MLO_Boolean
MLO_AdtsHeader_MatchFixed(unsigned char* a, unsigned char* b)
{
    if ( a[0]         ==  b[0] &&
         a[1]         ==  b[1] &&
         a[2]         ==  b[2] &&
        (a[3] & 0xF0) == (b[3] & 0xF0)) {
        return MLO_TRUE;
    } else {
        return MLO_FALSE;
    }
}

/*----------------------------------------------------------------------+
|        MLO_AdtsHeader_Parser
+----------------------------------------------------------------------*/
static void
MLO_AdtsHeader_Parse(MLO_AdtsHeader* header, unsigned char* bytes)
{
    /* fixed part */
    header->id                       = ( bytes[1] & 0x08) >> 3;
    header->protection_absent        =   bytes[1] & 0x01;
    header->profile_object_type      = ( bytes[2] & 0xC0) >> 6;
    header->sampling_frequency_index = ( bytes[2] & 0x3C) >> 2;
    header->channel_configuration    = ((bytes[2] & 0x01) << 2) | 
                                       ((bytes[3] & 0xC0) >> 6);
    /* variable part */
    header->frame_length = ((unsigned int)(bytes[3] & 0x03) << 11) |
                           ((unsigned int)(bytes[4]       ) <<  3) |
                           ((unsigned int)(bytes[5] & 0xE0) >>  5);
    header->raw_data_blocks =              bytes[6] & 0x03;
}

/*----------------------------------------------------------------------+
|        MLO_AdtsHeader_Check
+----------------------------------------------------------------------*/
static MLO_Result
MLO_AdtsHeader_Check(MLO_AdtsHeader* header)
{
    /* check that the sampling frequency index is valid */
    if (header->sampling_frequency_index >= 0xD) {
        return MLO_FAILURE;
    }

    /* MPEG2 does not use all profiles */
    if (header->id == 1 && header->profile_object_type == 3) {
        return MLO_FAILURE;
    }

    return MLO_SUCCESS;
}

/*----------------------------------------------------------------------+
|        MLO_AdtsParser_Create
+----------------------------------------------------------------------*/
MLO_Result
MLO_AdtsParser_Create(MLO_AdtsParser** parser)
{
    /* allocate memory for the object */
    *parser = (MLO_AdtsParser*)MLO_AllocateZeroMemory(sizeof(MLO_AdtsParser));

    /* construct the object */
    MLO_BitStream_AllocateBuffer(&(*parser)->bits);
    MLO_BitStream_Reset(&(*parser)->bits);

    return MLO_SUCCESS;
}

/*----------------------------------------------------------------------+
|        MLO_AdtsParser_Destroy
+----------------------------------------------------------------------*/
MLO_Result
MLO_AdtsParser_Destroy(MLO_AdtsParser* parser)
{
    /* destruct the object */
    MLO_BitStream_FreeBuffer(&parser->bits);

    /* free the memory */
    MLO_FreeMemory((void*)parser);

    return MLO_SUCCESS;
}

/*----------------------------------------------------------------------+
|        MLO_AdtsParser_Reset
+----------------------------------------------------------------------*/
MLO_Result
MLO_AdtsParser_Reset(MLO_AdtsParser* parser)
{
    parser->frame_count = 0;

    return MLO_SUCCESS;
}

/*----------------------------------------------------------------------+
|        MLO_AdtsParser_Feed
+----------------------------------------------------------------------*/
MLO_Result
MLO_AdtsParser_Feed(MLO_AdtsParser* parser, 
                    unsigned char*  buffer, 
                    MLO_Size*       size,
                    MLO_Flags       flags)
{
    MLO_Size free_space;

    /* possible shortcut */
    if (*size == 0) return MLO_SUCCESS;

    /* see how much data we can write */
    free_space = MLO_BitStream_GetBytesFree(&parser->bits);
    if (*size > free_space) *size = free_space;

    /* write the data */
    return MLO_BitStream_WriteBytes(&parser->bits, buffer, *size); 
}

/*----------------------------------------------------------------------+
|        MLO_AdtsParser_FindHeader
+----------------------------------------------------------------------*/
static MLO_Result
MLO_AdtsParser_FindHeader(MLO_AdtsParser* parser, unsigned char* header)
{
   int          available = MLO_BitStream_GetBytesAvailable(&parser->bits);
   unsigned int sync = 0;
   long          nbr_skipped_bytes = 0;

   /* look for the sync pattern */
   while (available-- >= MLO_ADTS_HEADER_SIZE)
   {
      sync = MLO_BitStream_ReadByte (&parser->bits) << 8;
      sync |= MLO_BitStream_PeekByte (&parser->bits);

      if ((sync & MLO_ADTS_SYNC_MASK) == MLO_ADTS_SYNC_PATTERN)
      {
         /* found a sync pattern, read the rest of the header */
         header[0] = (sync >> 8) & 0xFF;
         MLO_BitStream_ReadBytes(&parser->bits, &header[1], MLO_ADTS_HEADER_SIZE-1);

         return MLO_SUCCESS;
      }

      else
      {
         ++ nbr_skipped_bytes;
      }
   }

   return MLO_ERROR_NOT_ENOUGH_DATA;
}

/*----------------------------------------------------------------------+
|        MLO_AdtsParser_FindFrame
+----------------------------------------------------------------------*/
MLO_Result
MLO_AdtsParser_FindFrame(MLO_AdtsParser* parser, MLO_FrameData* frame)
{
    unsigned int   available;
    unsigned char  raw_header[MLO_ADTS_HEADER_SIZE];
    MLO_AdtsHeader adts_header;
    MLO_BitStream* bits = &parser->bits;
    MLO_Result     result;

    /* align to the start of the next byte */
    MLO_BitStream_ByteAlign(bits);
    
    /* find a frame header */
    result = MLO_AdtsParser_FindHeader(parser, raw_header);
    if (MLO_FAILED(result)) return result;

    /* parse the header */
    MLO_AdtsHeader_Parse(&adts_header, raw_header);

    /* check the header */
    result = MLO_AdtsHeader_Check(&adts_header);
    if (MLO_FAILED(result)) goto fail;
    
    /* check that we have enough data to peek at the next header */
    available = MLO_ADTS_HEADER_SIZE + MLO_BitStream_GetBytesAvailable(bits);
    if (bits->flags & MLO_BITSTREAM_FLAG_EOS) {
        /* we're at the end of the stream, we only need the entire frame */
        if (available < adts_header.frame_length) {
            return MLO_ERROR_NOT_ENOUGH_DATA;
        } 
    } else {
        /* peek at the header of the next frame */
        unsigned char peek_raw_header[MLO_ADTS_HEADER_SIZE];
        MLO_AdtsHeader peek_adts_header;

        if (available < adts_header.frame_length+MLO_ADTS_HEADER_SIZE) {
            return MLO_ERROR_NOT_ENOUGH_DATA;
        } 
        MLO_BitStream_SkipBytes(bits, adts_header.frame_length-MLO_ADTS_HEADER_SIZE);
        MLO_BitStream_PeekBytes(bits, peek_raw_header, MLO_ADTS_HEADER_SIZE);
        MLO_BitStream_SkipBytes(bits, -((int)adts_header.frame_length-MLO_ADTS_HEADER_SIZE));

        /* check the header */
        MLO_AdtsHeader_Parse(&peek_adts_header, peek_raw_header);
        result = MLO_AdtsHeader_Check(&peek_adts_header);
        if (MLO_FAILED(result)) goto fail;

        /* check that the fixed part of this header is the same as the */
        /* fixed part of the previous header                           */
        if (!MLO_AdtsHeader_MatchFixed(peek_raw_header, raw_header)) {
            goto fail;
        }
    }

    /* fill in the frame info */
    frame->info.standard = (adts_header.id == 1 ? 
                            MLO_AAC_STANDARD_MPEG2 :
                            MLO_AAC_STANDARD_MPEG4);
    switch (adts_header.profile_object_type) {
        case 0:
            frame->info.profile = MLO_AAC_PROFILE_MAIN;
            break;

        case 1:
            frame->info.profile = MLO_AAC_PROFILE_LC;
            break;

        case 2: 
            frame->info.profile = MLO_AAC_PROFILE_SSR;
            break;

        case 3:
            frame->info.profile = MLO_AAC_PROFILE_LTP;
    }
    frame->info.frame_length = adts_header.frame_length-MLO_ADTS_HEADER_SIZE;
    frame->info.channel_configuration = adts_header.channel_configuration;
    frame->info.sampling_frequency_index = adts_header.sampling_frequency_index;
    frame->info.sampling_frequency = MLO_SamplingFreq_table [adts_header.sampling_frequency_index];

    /* skip crc if present */
    if (adts_header.protection_absent == 0) {
        MLO_BitStream_SkipBits(bits, 16);
    } 

    /* set the frame source */
    frame->source = bits;

    return MLO_SUCCESS;

fail:
    /* skip the header and return (only skip the first byte in) */
    /* case this was a false header that hides one just after)  */
    MLO_BitStream_SkipBytes(bits, -(MLO_ADTS_HEADER_SIZE-1));
    return MLO_ERROR_CORRUPTED_BITSTREAM;
}



unsigned int	
MLO_AdtsParser_GetBytesFree (const MLO_AdtsParser* parser)
{
	MLO_ASSERT (parser != 0);

	return (MLO_BitStream_GetBytesFree (&parser->bits));
}



unsigned int	
MLO_AdtsParser_GetBytesAvailable (const MLO_AdtsParser* parser)
{
	MLO_ASSERT (parser != 0);

	return (MLO_BitStream_GetBytesAvailable(&parser->bits));
}



#endif
