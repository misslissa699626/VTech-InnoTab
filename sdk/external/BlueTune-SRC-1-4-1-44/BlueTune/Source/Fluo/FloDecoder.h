/*****************************************************************
|
|   Fluo - Decoder API
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Fluo Decoder API
 */

#ifndef _FLO_DECODER_H_
#define _FLO_DECODER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "FloFrame.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct FLO_Decoder FLO_Decoder;

typedef enum {
    FLO_SAMPLE_TYPE_INTERLACED_SIGNED
} FLO_SampleType;

typedef struct {
    FLO_SampleType type;
    FLO_Cardinal   sample_rate;
    FLO_Cardinal   channel_count;
    FLO_Cardinal   bits_per_sample;
} FLO_SampleFormat;

typedef struct {
    FLO_Size         size;
    FLO_Any          samples;
    FLO_Cardinal     sample_count;
    FLO_SampleFormat format;
} FLO_SampleBuffer;

typedef struct {
    FLO_Flags    flags;
    FLO_Cardinal frame_count;
    FLO_Int64    sample_count;
    struct {
        FLO_Size     size;
        FLO_Cardinal bitrate;
        FLO_Int64    duration_ms;
        FLO_Cardinal duration_frames;
        FLO_Int64    duration_samples;
        FLO_Cardinal decoder_delay;
        FLO_Cardinal encoder_delay;
        FLO_Cardinal encoder_padding;
    }            stream_info;
    struct {
        FLO_Flags flags;
        FLO_Int32 album_gain;
        FLO_Int32 track_gain;
    }            replay_gain_info;
    FLO_Cardinal vbr_quality;
} FLO_DecoderStatus;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define FLO_DECODER_BUFFER_IS_DISCONTINUOUS     0x01
#define FLO_DECODER_BUFFER_IS_START_OF_STREAM   0x02
#define FLO_DECODER_BUFFER_IS_END_OF_STREAM     0x04

#define FLO_DECODER_STATUS_STREAM_IS_VBR          0x01
#define FLO_DECODER_STATUS_STREAM_HAS_INFO        0x02
#define FLO_DECODER_STATUS_STREAM_HAS_SEEK_TABLE  0x04
#define FLO_DECODER_STATUS_STREAM_HAS_REPLAY_GAIN 0x08

#define FLO_REPLAY_GAIN_HAS_TRACK_VALUE 1
#define FLO_REPLAY_GAIN_HAS_ALBUM_VALUE 2

#define FLO_FRAME_BUFFER_SIZE    2048

/*----------------------------------------------------------------------
|   error codes
+---------------------------------------------------------------------*/
#define FLO_ERROR_INVALID_DECODER_STATE (FLO_ERROR_BASE_DECODER - 0)
#define FLO_ERROR_FRAME_SKIPPED         (FLO_ERROR_BASE_DECODER - 1)
#define FLO_ERROR_SAMPLES_SKIPPED       (FLO_ERROR_BASE_DECODER - 2)
#define FLO_ERROR_NO_MORE_SAMPLES       (FLO_ERROR_BASE_DECODER - 3)
#define FLO_ERROR_NOT_ENOUGH_DATA       (FLO_ERROR_BASE_DECODER - 4)
#define FLO_ERROR_INVALID_BITSTREAM     (FLO_ERROR_BASE_DECODER - 5)

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
FLO_Result FLO_Decoder_Create(FLO_Decoder** decoder);
FLO_Result FLO_Decoder_Destroy(FLO_Decoder* decoder);
FLO_Result FLO_Decoder_Reset(FLO_Decoder* decoder, FLO_Boolean new_stream);
FLO_Result FLO_Decoder_Feed(FLO_Decoder*   decoder, 
                            FLO_ByteBuffer buffer, 
                            FLO_Size*      size,
                            FLO_Flags      flags);
FLO_Result FLO_Decoder_Flush(FLO_Decoder* decoder);
FLO_Result FLO_Decoder_SetSample(FLO_Decoder* decoder,
                                 FLO_Int64    sample);
FLO_Result FLO_Decoder_FindFrame(FLO_Decoder*   decoder, 
                                 FLO_FrameInfo* frame_info);
FLO_Result FLO_Decoder_SkipFrame(FLO_Decoder* decoder);
FLO_Result FLO_Decoder_DecodeFrame(FLO_Decoder*      decoder,
                                   FLO_SampleBuffer* buffer,
                                   FLO_Cardinal*     samples_skipped);
FLO_Result FLO_Decoder_GetStatus(FLO_Decoder*        decoder, 
                                 FLO_DecoderStatus** status);
FLO_Result FLO_Decoder_GetSeekPoint(FLO_Decoder* decoder,
                                    FLO_Offset   target_offset,
                                    FLO_Size     target_range,
                                    FLO_Offset*  seek_offset,
                                    FLO_Size*    seek_range);

#endif /* _FLO_DECODER_H_ */
