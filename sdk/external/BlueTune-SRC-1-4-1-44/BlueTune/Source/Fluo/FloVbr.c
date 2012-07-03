/*****************************************************************
|
|      File: FloVbr.c
|
|      Fluo - Vbr (Variable Bitrate)
|
|      (c) 2002-2007 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "FloConfig.h"
#include "FloVbr.h"
#include "FloDecoder.h"
#include "FloByteStream.h"

/*----------------------------------------------------------------------
|       constants
+---------------------------------------------------------------------*/
#define FLO_FHG_VBR_HEADER_OFFSET               36
#define FLO_FHG_VBR_HEADER_SIZE                 26
#define FLO_FHG_VBR_HEADER_EXPECTED_VERSION     1

#define FLO_XING_VBR_HEADER_SIZE                16
#define FLO_XING_VBR_HEADER_HAS_FRAME_COUNT     0x0001
#define FLO_XING_VBR_HEADER_HAS_BYTE_COUNT      0x0002
#define FLO_XING_VBR_HEADER_HAS_TOC             0x0004
#define FLO_XING_VBR_HEADER_HAS_VBR_SCALE       0x0008
#define FLO_XING_VBR_TOC_SIZE                   100

/*----------------------------------------------------------------------
|       FLO_Vbr_ParseFhg
+---------------------------------------------------------------------*/
static void
FLO_Vbr_ComputeDurationAndBitrate(FLO_FrameInfo*     frame_info,
                                  FLO_DecoderStatus* decoder_status)
{
    if (frame_info->sample_rate) {
        ATX_UInt64 sample_count = (ATX_UInt64)decoder_status->stream_info.duration_frames *
                                  (ATX_UInt64)frame_info->sample_count;
        ATX_UInt64 duration_ms = (1000*sample_count)/frame_info->sample_rate;

        decoder_status->stream_info.duration_ms = duration_ms;

        /* compute the bitrate */
        if (decoder_status->stream_info.duration_ms) {
            ATX_UInt64 bitrate = (8*1000*(ATX_UInt64)decoder_status->stream_info.size)/duration_ms;
            decoder_status->stream_info.bitrate = (ATX_UInt32)(bitrate);
        } else {
            decoder_status->stream_info.bitrate = 0;
        }
    } else {
        /* we cannot compute the duration or bitrate */
        decoder_status->stream_info.duration_ms = 0;
        decoder_status->stream_info.bitrate     = 0;
    }
}        

/*----------------------------------------------------------------------
|       FLO_Vbr_ParseFhg
+---------------------------------------------------------------------*/
static FLO_Result 
FLO_Vbr_ParseFhg(FLO_FrameInfo*     frame_info,
                 FLO_ByteStream*    bits,
                 FLO_DecoderStatus* decoder_status,
                 FLO_VbrToc*        vbr_toc)
{
    unsigned char  buffer[FLO_FHG_VBR_HEADER_SIZE];
    unsigned char* current = buffer;
    FLO_ByteStream scan;

    /* check the frame size */
    if (frame_info->size < FLO_FHG_VBR_HEADER_OFFSET+FLO_FHG_VBR_HEADER_SIZE) {
        return FLO_FAILURE;
    }

    /* attach to the bitstream and read the header */
    FLO_ByteStream_Attach(bits, &scan);
    FLO_ByteStream_SkipBytes(&scan, FLO_FHG_VBR_HEADER_OFFSET);
    FLO_ByteStream_ReadBytes(&scan, buffer, sizeof(buffer));

    /* check header cookie */
    if (current[0] != 'V' ||
        current[1] != 'B' ||
        current[2] != 'R' ||
        current[3] != 'I') {
        return FLO_FAILURE;
    } 
    current += 4;

    /* check header version */
    if (ATX_BytesToInt16Be(current) != FLO_FHG_VBR_HEADER_EXPECTED_VERSION) {
        return FLO_FAILURE;
    }
    current += 2;

    /* this is a valid VBR header */
    decoder_status->flags |= FLO_DECODER_STATUS_STREAM_IS_VBR;
    decoder_status->flags |= FLO_DECODER_STATUS_STREAM_HAS_INFO;

    /* skip the delay field (no use for it) */
    current += 2;

    /* quality */
    decoder_status->vbr_quality = ATX_BytesToInt16Be(current);
    current += 2;
    
    /* stream size */
    decoder_status->stream_info.size = ATX_BytesToInt32Be(current);
    current += 4;

    /* frame count */
    decoder_status->stream_info.duration_frames = ATX_BytesToInt32Be(current);

    /* compute duration and bitrate */
    FLO_Vbr_ComputeDurationAndBitrate(frame_info, decoder_status);

    /* seek table (ignored for now) */
    vbr_toc->entry_count = 0;
    
    return FLO_SUCCESS;
}

/*----------------------------------------------------------------------
|       FLO_Vbr_ParseXing
+---------------------------------------------------------------------*/
static FLO_Result 
FLO_Vbr_ParseXing(FLO_FrameInfo*     frame_info,
                  FLO_ByteStream*    bits,
                  FLO_DecoderStatus* decoder_status,
                  FLO_VbrToc*        vbr_toc)
{
    unsigned char  buffer[FLO_XING_VBR_HEADER_SIZE];
    unsigned char  toc[FLO_XING_VBR_TOC_SIZE];
    unsigned char* current = buffer;
    FLO_Size       header_offset;
    FLO_Flags      header_flags;
    FLO_ByteStream scan;

    /* compute header offset */
    if (frame_info->level == FLO_SYNTAX_MPEG_ID_MPEG_1) {
        if (frame_info->mode == FLO_SYNTAX_MPEG_MODE_SINGLE_CHANNEL) {
            header_offset = 4+17;
        } else {
            header_offset = 4+32;
        }
    } else if (frame_info->level == FLO_SYNTAX_MPEG_ID_MPEG_2) {
        if (frame_info->mode == FLO_SYNTAX_MPEG_MODE_SINGLE_CHANNEL) {
            header_offset = 4+9;
        } else {
            header_offset = 4+17;
        }
    } else {
        return FLO_FAILURE;
    }

    /* check the frame size */
    if (frame_info->size < header_offset + FLO_XING_VBR_HEADER_SIZE) {
        return FLO_FAILURE;
    }

    /* attach to the bitstream and read the header */
    FLO_ByteStream_Attach(bits, &scan);
    FLO_ByteStream_SkipBytes(&scan, header_offset);
    FLO_ByteStream_ReadBytes(&scan, buffer, sizeof(buffer));

    /* check header cookie */
    if ((current[0] != 'X' ||
         current[1] != 'i' ||
         current[2] != 'n' ||
         current[3] != 'g') && 
        (current[0] != 'L' ||
         current[1] != 'a' ||
         current[2] != 'm' ||
         current[3] != 'e')) {
        return FLO_FAILURE;
    } 
    current += 4;

    /* this is a valid VBR header */
    decoder_status->flags |= FLO_DECODER_STATUS_STREAM_IS_VBR;
    decoder_status->flags |= FLO_DECODER_STATUS_STREAM_HAS_INFO;

    /* header flags */
    header_flags = ATX_BytesToInt32Be(current);
    current += 4;

    /* frame count */
    if (header_flags & FLO_XING_VBR_HEADER_HAS_FRAME_COUNT) {
        decoder_status->stream_info.duration_frames = 
            ATX_BytesToInt32Be(current);
        current += 4;
    } else {
        decoder_status->stream_info.duration_frames = 0;
    }
    
    /* stream size */
    if (header_flags & FLO_XING_VBR_HEADER_HAS_BYTE_COUNT) {
        decoder_status->stream_info.size = ATX_BytesToInt32Be(current);
    } else {
        decoder_status->stream_info.size = 0;
    }

    /* toc */
    if (header_flags & FLO_XING_VBR_HEADER_HAS_TOC) {
        FLO_ByteStream_ReadBytes(&scan, toc, FLO_XING_VBR_TOC_SIZE);
    }

    /* vbr scale */
    if (header_flags & FLO_XING_VBR_HEADER_HAS_VBR_SCALE) {
        FLO_ByteStream_ReadBytes(&scan, buffer, 4);
        decoder_status->vbr_quality = ATX_BytesToInt32Be(buffer);
    } else {
        decoder_status->vbr_quality = 0;
    }

    /* compute duration and bitrate */
    FLO_Vbr_ComputeDurationAndBitrate(frame_info, decoder_status);

    /* seek table (ignored for now) */
    vbr_toc->entry_count = 0;

    return FLO_SUCCESS;
}

/*----------------------------------------------------------------------
|       FLO_Vbr_Parse
+---------------------------------------------------------------------*/
FLO_Result 
FLO_Vbr_Parse(FLO_FrameInfo*     frame_info,
              FLO_ByteStream*    bits,
              FLO_DecoderStatus* decoder_status,
              FLO_VbrToc*        vbr_toc)
{
    FLO_Result result;

    /* first, check for an FHG VBRI header */
    result = FLO_Vbr_ParseFhg(frame_info, bits, decoder_status, vbr_toc);
    if (FLO_SUCCEEDED(result)) return FLO_SUCCESS;

    /* first, check for a Xing/Lame VBR header */
    result = FLO_Vbr_ParseXing(frame_info, bits, decoder_status, vbr_toc);
    if (FLO_SUCCEEDED(result)) return FLO_SUCCESS;

    return FLO_FAILURE;
}


