/*****************************************************************
|
|   Fluo - Vbr (Variable Bitrate)
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "FloConfig.h"
#include "FloHeaders.h"
#include "FloDecoder.h"
#include "FloByteStream.h"

/*----------------------------------------------------------------------
|   constants
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

#define FLO_LAME_TAG_SIZE                         36
#define FLO_LAME_TAG_REVISION_OFFSET              9
#define FLO_LAME_TAG_DELAY_INFO_OFFSET            21
#define FLO_LAME_TAG_REPLAYGAIN_TRACK_GAIN_OFFSET 15
#define FLO_LAME_TAG_REPLAYGAIN_ALBUM_GAIN_OFFSET 17

#define FLO_LAME_TAG_REVISION_MASK              0x0F
#define FLO_LAME_TAG_REPLAYGAIN_CODE_RADIO      1
#define FLO_LAME_TAG_REPLAYGAIN_CODE_AUDIOPHILE 2
#define FLO_LAME_TAG_REPLAYGAIN_ORIGIN_NOT_SET  0

#define FLO_REPLAYGAIN_MIN_VALUE                -250 /* -25.0dB */
#define FLO_REPLAYGAIN_MAX_VALUE                 200  /* +20.0dB */

/*----------------------------------------------------------------------
|   FLO_Vbr_ComputeDurationAndBitrate
+---------------------------------------------------------------------*/
static void
FLO_Vbr_ComputeDurationAndBitrate(FLO_FrameInfo*     frame_info,
                                  FLO_DecoderStatus* decoder_status)
{
    /* compute the number of samples */
    ATX_UInt64 sample_count = (ATX_UInt64)decoder_status->stream_info.duration_frames *
                              frame_info->sample_count;
    sample_count -= (decoder_status->stream_info.encoder_delay + decoder_status->stream_info.encoder_padding);
    decoder_status->stream_info.duration_samples = sample_count;

    if (frame_info->sample_rate) {
        /* compute the duration in ms */
        ATX_UInt64 duration = (1000*(ATX_UInt64)sample_count)/frame_info->sample_rate;
        decoder_status->stream_info.duration_ms = duration;

        /* compute the bitrate */
        if (decoder_status->stream_info.duration_ms) {
            ATX_UInt64 bitrate = (8*1000*(ATX_UInt64)(decoder_status->stream_info.size))/duration;
            decoder_status->stream_info.bitrate = (ATX_UInt32)(bitrate);
        } else {
            decoder_status->stream_info.bitrate = 0;
        }
    } else {
        /* we cannot compute the duration in ms, or bitrate */
        decoder_status->stream_info.duration_ms = 0;
        decoder_status->stream_info.bitrate     = 0;
    }
}        

/*----------------------------------------------------------------------
|   FLO_Vbr_ParseFhg
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
|   FLO_Headers_ParseXingOrLame
+---------------------------------------------------------------------*/
static FLO_Result 
FLO_Headers_ParseXingOrLame(FLO_FrameInfo*     frame_info,
                            FLO_ByteStream*    bits,
                            FLO_DecoderStatus* decoder_status,
                            FLO_VbrToc*        vbr_toc)
{
    unsigned char  buffer[FLO_XING_VBR_HEADER_SIZE];
    unsigned char  toc[FLO_XING_VBR_TOC_SIZE];
    unsigned char  lame_tag[FLO_LAME_TAG_SIZE];
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

    /* check header cookie (Xing|Lame|Info) */
    if ((current[0] != 'X' ||
         current[1] != 'i' ||
         current[2] != 'n' ||
         current[3] != 'g') && 
        (current[0] != 'L' ||
         current[1] != 'a' ||
         current[2] != 'm' ||
         current[3] != 'e') &&
        (current[0] != 'I' ||
         current[1] != 'n' ||
         current[2] != 'f' ||
         current[3] != 'o')) {
        return FLO_FAILURE;
    } 

    /* this is a valid VBR header */
    if (current[0] != 'I') {
        /* the 'Info' cookie means LAME CBR, else it's VBR */
        decoder_status->flags |= FLO_DECODER_STATUS_STREAM_IS_VBR;
    }
    decoder_status->flags |= FLO_DECODER_STATUS_STREAM_HAS_INFO;

    /* skip cookie */
    current += 4;

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
    if (frame_info->size < header_offset + FLO_XING_VBR_HEADER_SIZE + 100) {
        goto done;
    }
    if (header_flags & FLO_XING_VBR_HEADER_HAS_TOC) {
        FLO_ByteStream_ReadBytes(&scan, toc, FLO_XING_VBR_TOC_SIZE);
    }

    /* vbr scale */
    if (frame_info->size < header_offset + FLO_XING_VBR_HEADER_SIZE + 100 + 4) {
        goto done;
    }
    if (header_flags & FLO_XING_VBR_HEADER_HAS_VBR_SCALE) {
        FLO_ByteStream_ReadBytes(&scan, buffer, 4);
        decoder_status->vbr_quality = ATX_BytesToInt32Be(buffer);
    } else {
        decoder_status->vbr_quality = 0;
    }

    /* look for LAME tags */
    if (frame_info->size < header_offset + 
                           FLO_XING_VBR_HEADER_SIZE + 100 + 4 + 
                           FLO_LAME_TAG_SIZE) {
        goto done;
    }
    FLO_ByteStream_ReadBytes(&scan, lame_tag, sizeof(lame_tag));
    if (lame_tag[0] == 'L' &&
        lame_tag[1] == 'A' &&
        lame_tag[2] == 'M' &&
        lame_tag[3] == 'E') {
        /* this is a LAME tag */
        unsigned char tag_revision = 
            lame_tag[FLO_LAME_TAG_REVISION_OFFSET] &
            FLO_LAME_TAG_REVISION_MASK;
        if (tag_revision >= 1) {
            unsigned int delay = 
                (lame_tag[FLO_LAME_TAG_DELAY_INFO_OFFSET] << 4) |
                ((lame_tag[FLO_LAME_TAG_DELAY_INFO_OFFSET+1] & 0xF0) >> 4);
            unsigned int padding =
                ((lame_tag[FLO_LAME_TAG_DELAY_INFO_OFFSET+1] & 0x0F) << 8) |
                (lame_tag[FLO_LAME_TAG_DELAY_INFO_OFFSET+2]);
            if (delay <= 1152 && padding <= 1152*2 &&
                decoder_status->stream_info.duration_frames*1152 >= delay+padding) {
                /* the values look ok */
                decoder_status->stream_info.encoder_delay = delay;
                decoder_status->stream_info.encoder_padding = padding;
            }

            /* parse the replay gain track info */
            {
                unsigned char track_gain_code = 
                    (lame_tag[FLO_LAME_TAG_REPLAYGAIN_TRACK_GAIN_OFFSET]>>5)&0x7;
                unsigned char track_gain_origin = 
                    (lame_tag[FLO_LAME_TAG_REPLAYGAIN_TRACK_GAIN_OFFSET]>>2)&0x7;
                unsigned char track_gain_sign =
                    lame_tag[FLO_LAME_TAG_REPLAYGAIN_TRACK_GAIN_OFFSET]&2;
                int track_gain_value =
                    ((lame_tag[FLO_LAME_TAG_REPLAYGAIN_TRACK_GAIN_OFFSET  ]&1)<<7) |
                      lame_tag[FLO_LAME_TAG_REPLAYGAIN_TRACK_GAIN_OFFSET+1];

                unsigned char album_gain_code = 
                    (lame_tag[FLO_LAME_TAG_REPLAYGAIN_ALBUM_GAIN_OFFSET]>>5)&0x7;
                unsigned char album_gain_origin = 
                    (lame_tag[FLO_LAME_TAG_REPLAYGAIN_ALBUM_GAIN_OFFSET]>>2)&0x7;
                unsigned char album_gain_sign =
                    lame_tag[FLO_LAME_TAG_REPLAYGAIN_ALBUM_GAIN_OFFSET]&2;
                int album_gain_value =
                    ((lame_tag[FLO_LAME_TAG_REPLAYGAIN_ALBUM_GAIN_OFFSET  ]&1)<<7) |
                      lame_tag[FLO_LAME_TAG_REPLAYGAIN_ALBUM_GAIN_OFFSET+1];

                /* set the track gain info if it is present */
                if (track_gain_code == FLO_LAME_TAG_REPLAYGAIN_CODE_RADIO &&
                    track_gain_origin != FLO_LAME_TAG_REPLAYGAIN_ORIGIN_NOT_SET) {
                    FLO_Int32 gain = track_gain_sign ? -track_gain_value : track_gain_value;
                    if (gain >= FLO_REPLAYGAIN_MIN_VALUE && 
                        gain <= FLO_REPLAYGAIN_MAX_VALUE) {
                        decoder_status->replay_gain_info.flags |= FLO_REPLAY_GAIN_HAS_TRACK_VALUE;
                        decoder_status->replay_gain_info.track_gain = gain;
                        decoder_status->flags |= FLO_DECODER_STATUS_STREAM_HAS_REPLAY_GAIN;
                    }
                }

                /* set the album gain info if it is present */
                if (album_gain_code == FLO_LAME_TAG_REPLAYGAIN_CODE_RADIO &&
                    album_gain_origin != FLO_LAME_TAG_REPLAYGAIN_ORIGIN_NOT_SET) {
                    FLO_Int32 gain = album_gain_sign ? -album_gain_value : album_gain_value;
                    if (gain >= FLO_REPLAYGAIN_MIN_VALUE && 
                        gain <= FLO_REPLAYGAIN_MAX_VALUE) {
                        decoder_status->replay_gain_info.flags |= FLO_REPLAY_GAIN_HAS_ALBUM_VALUE;
                        decoder_status->replay_gain_info.album_gain = gain;
                        decoder_status->flags |= FLO_DECODER_STATUS_STREAM_HAS_REPLAY_GAIN;
                    }
                }
            }
        }
    }

done:
    /* compute duration and bitrate */
    FLO_Vbr_ComputeDurationAndBitrate(frame_info, decoder_status);

    /* seek table (ignored for now) */
    vbr_toc->entry_count = 0;

    return FLO_SUCCESS;
}

/*----------------------------------------------------------------------
|   FLO_Headers_Parse
+---------------------------------------------------------------------*/
FLO_Result 
FLO_Headers_Parse(FLO_FrameInfo*     frame_info,
                  FLO_ByteStream*    bits,
                  FLO_DecoderStatus* decoder_status,
                  FLO_VbrToc*        vbr_toc)
{
    FLO_Result result;

    /* first, check for an FHG VBRI header */
    result = FLO_Vbr_ParseFhg(frame_info, bits, decoder_status, vbr_toc);
    if (FLO_SUCCEEDED(result)) return FLO_SUCCESS;

    /* first, check for a Xing/Lame VBR or CBR header */
    result = FLO_Headers_ParseXingOrLame(frame_info, bits, decoder_status, vbr_toc);
    if (FLO_SUCCEEDED(result)) return FLO_SUCCESS;

    return FLO_FAILURE;
}


