/*****************************************************************
|
|   Fluo - Frame Decoding Engine
|
|   (c) 2002-20076 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "FloConfig.h"
#include "FloEngine.h"
#include "FloFilter.h"
#include "FloLayerI.h"
#include "FloLayerII.h"
#include "FloLayerIII.h"
#include "FloUtils.h"
#include "FloErrors.h"

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#if (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_MPG123)
#include "mpg123.h"
#include "mpglib.h"
#elif (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_FFMPEG)
#include <stdio.h>
#include "avcodec.h"
#elif (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN)
#else
#error FLO_DECODER_ENGINE not defined
#endif

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
#if (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_MPG123)
struct FLO_Engine {
    MPSTR mpg123_decoder;
};
#elif (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_FFMPEG)
struct FLO_Engine {
    AVCodec*        ffmpeg_decoder_module;
    AVCodecContext* ffmpeg_decoder_context;
};
#elif (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN)
typedef enum { 
    FLO_OUTPUT_STEREO, 
    FLO_OUTPUT_MONO_LEFT,         
    FLO_OUTPUT_MONO_RIGHT,
    FLO_OUTPUT_MONO_MIX
} FLO_OutputChannelsMode;

typedef struct {
    FLO_OutputChannelsMode channels;
} FLO_EngineConfig;

struct FLO_Engine {
    FLO_EngineConfig     config;
    FLO_SynthesisFilter* left_filter;
    FLO_SynthesisFilter* right_filter;
    union {
        FLO_FrameHeader  header;
        FLO_Frame_I      frame_I;
        FLO_Frame_II     frame_II;
        FLO_Frame_III    frame_III;
    } frame;
    FLO_MainDataBuffer   main_data;
};
#endif

/*----------------------------------------------------------------------
|   FLO_Engine_Construct
+---------------------------------------------------------------------*/
#if (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_MPG123)
static FLO_Result 
FLO_Engine_Construct(FLO_Engine* self)
{
    /* initialize the mpg123 library */
    MPGLIB_Init(&self->mpg123_decoder);

    return FLO_SUCCESS;
}
#elif (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_FFMPEG)
static FLO_Result 
FLO_Engine_Construct(FLO_Engine* self)
{
    /* initialize the ffmpeg decoder */
    self->ffmpeg_decoder_context = avcodec_alloc_context();
    self->ffmpeg_decoder_module = &mp3_decoder;
    avcodec_open(self->ffmpeg_decoder_context, 
                 self->ffmpeg_decoder_module);

    return FLO_SUCCESS;
}
#elif (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN)
static FLO_Result 
FLO_Engine_Construct(FLO_Engine* self)
{
    FLO_SynthesisFilter_Create(&self->left_filter);
    FLO_SynthesisFilter_Create(&self->right_filter);
    FLO_LayerIII_ResetFrame(&self->frame.frame_III);
    self->config.channels = FLO_OUTPUT_STEREO;
    self->main_data.available = 0;
    return FLO_SUCCESS;
}
#endif

/*----------------------------------------------------------------------
|   FLO_Engine_Create
+---------------------------------------------------------------------*/
FLO_Result
FLO_Engine_Create(FLO_Engine** engine)
{
    FLO_Result result;

    /* allocate the object */
    *engine = (FLO_Engine*)FLO_AllocateZeroMemory(sizeof(FLO_Engine));
    if (*engine == NULL) return FLO_ERROR_OUT_OF_MEMORY;

    /* construct the object */
    result = FLO_Engine_Construct(*engine);
    if (FLO_FAILED(result)) {
        FLO_FreeMemory(*engine);
        *engine = NULL;
        return result;
    }

    return FLO_SUCCESS;
}

/*----------------------------------------------------------------------
|   FLO_Engine_Destruct
+---------------------------------------------------------------------*/
#if (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_MPG123)
static void
FLO_Engine_Destruct(FLO_Engine* self) 
{
    MPGLIB_Exit(&self->mpg123_decoder);
}
#elif (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_FFMPEG)
static void
FLO_Engine_Destruct(FLO_Engine* self) 
{
    avcodec_close(self->ffmpeg_decoder_context);
    free((void*)self->ffmpeg_decoder_context);
}
#elif (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN)
static void
FLO_Engine_Destruct(FLO_Engine* self) 
{
    FLO_SynthesisFilter_Destroy(self->left_filter);
    FLO_SynthesisFilter_Destroy(self->right_filter);
}
#endif

/*----------------------------------------------------------------------
|   FLO_Engine_Destroy
+---------------------------------------------------------------------*/
FLO_Result
FLO_Engine_Destroy(FLO_Engine* self)
{
    /* call the destructor */
    FLO_Engine_Destruct(self);

    /* free the memory */
    FLO_FreeMemory(self);

    return FLO_SUCCESS;
}

/*----------------------------------------------------------------------
|   FLO_Engine_DecodeFrame
+---------------------------------------------------------------------*/
#if (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_MPG123)
FLO_Result
FLO_Engine_DecodeFrame(FLO_Engine*          self, 
                       const FLO_FrameInfo* frame_info,
                       const unsigned char* frame_data, 
                       FLO_SampleBuffer*    sample_buffer)
{
    int result;

    result = MPGLIB_DecodeFrame(&self->mpg123_decoder,
                                frame_data,
                                sample_buffer->samples);

    /* default value */
    sample_buffer->size = 0;

    if (result == MP3_OK) {
        /* set the buffer info */
        sample_buffer->size = frame_info->channel_count *
                              frame_info->sample_count * 2;
        sample_buffer->sample_count           = frame_info->sample_count;
        sample_buffer->format.type            = FLO_SAMPLE_TYPE_INTERLACED_SIGNED;
        sample_buffer->format.sample_rate     = frame_info->sample_rate;
        sample_buffer->format.channel_count   = frame_info->channel_count;
        sample_buffer->format.bits_per_sample = 16;

        return FLO_SUCCESS;
    } else if (result == MP3_NEED_MORE) {
        return FLO_ERROR_NOT_ENOUGH_DATA;
    } else if (result == MP3_INVALID_BITS) {
        return FLO_ERROR_INVALID_BITSTREAM;
    } else {
        return FLO_FAILURE;
    }
}
#elif (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_FFMPEG)
FLO_Result
FLO_Engine_DecodeFrame(FLO_Engine*          self, 
                       const FLO_FrameInfo* frame_info,
                       const unsigned char* frame_data, 
                       FLO_SampleBuffer*    sample_buffer)
{
    FLO_Result result;
    int        ffmpeg_frame_size;

    avcodec_decode_audio(self->ffmpeg_decoder_context,
                         sample_buffer->samples,
                         &ffmpeg_frame_size,
                         frame_data,
                         frame_info->size);
    if (ffmpeg_frame_size == 0) {
        avcodec_decode_audio(self->ffmpeg_decoder_context,
                             sample_buffer->samples,
                             &ffmpeg_frame_size,
                             NULL, 0);
        if (ffmpeg_frame_size == 0) {
            result = FLO_FAILURE;
        } else {
            result = FLO_SUCCESS;
        }
    } else {
        result = FLO_SUCCESS;
    }

    if (result == FLO_SUCCESS) {
        /* set the buffer info */
        sample_buffer->size = frame_info->channel_count *
                              frame_info->sample_count * 2;
        sample_buffer->sample_count           = frame_info->sample_count;
        sample_buffer->format.type            = FLO_SAMPLE_TYPE_INTERLACED_SIGNED;
        sample_buffer->format.sample_rate     = frame_info->sample_rate;
        sample_buffer->format.channel_count   = frame_info->channel_count;
        sample_buffer->format.bits_per_sample = 16;
    } else {
        sample_buffer->size = 0;
    }

    return result;
}
#elif (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN)
FLO_Result
FLO_Engine_DecodeFrame(FLO_Engine*          self, 
                       const FLO_FrameInfo* frame_info,
                       const unsigned char* frame_data, 
                       FLO_SampleBuffer*    sample_buffer)
{
    FLO_SynthesisFilter* left_filter  = self->left_filter;
    FLO_SynthesisFilter* right_filter = self->right_filter;
    FLO_Result           result;

    /* read the header */
    FLO_FrameHeader_FromBytes(frame_data, &self->frame.header);
    frame_data += 4;

    /* skip the CRC, if any */
    if (self->frame.header.protection_bit == 0) {
        frame_data += 2;
    }
    
    /* setup the filters and audio buffer parameters */
    sample_buffer->sample_count           = frame_info->sample_count;
    sample_buffer->format.type            = FLO_SAMPLE_TYPE_INTERLACED_SIGNED;
    sample_buffer->format.sample_rate     = frame_info->sample_rate;
    sample_buffer->format.channel_count   = frame_info->channel_count;
    sample_buffer->format.bits_per_sample = 16;
    if (frame_info->mode == FLO_MPEG_MODE_SINGLE_CHANNEL) {
        right_filter = NULL;
        left_filter->buffer = sample_buffer->samples;
        left_filter->buffer_increment = 1;
    } else {
        switch (self->config.channels) {
          case FLO_OUTPUT_MONO_LEFT:
            right_filter = NULL;
            left_filter->buffer                 = sample_buffer->samples;
            left_filter->buffer_increment       = 1;
            sample_buffer->format.channel_count = 1;
            break;

          case FLO_OUTPUT_MONO_RIGHT:
            left_filter = NULL;
            right_filter->buffer                = sample_buffer->samples;
            right_filter->buffer_increment      = 1;
            sample_buffer->format.channel_count = 1;
            break;

          case FLO_OUTPUT_MONO_MIX: 
            right_filter = left_filter;
            left_filter->buffer                 = sample_buffer->samples;
            left_filter->buffer_increment       = 1;
            sample_buffer->format.channel_count = 1;
            break;

          case FLO_OUTPUT_STEREO:
            left_filter->buffer            = sample_buffer->samples;
            left_filter->buffer_increment  = 2;
            right_filter->buffer           = ((short*)sample_buffer->samples)+1;
            right_filter->buffer_increment = 2;
            break;
        }
    }
            
    /* decode the frame */
    switch (frame_info->layer) {

#ifndef FLO_CONFIG_MINI_BUILD
      case FLO_MPEG_LAYER_I:
        result = FLO_LayerI_DecodeFrame(frame_data, 
                                        frame_info,
                                        &self->frame.frame_I, 
                                        left_filter, 
                                        right_filter);
        break;
        
      case FLO_MPEG_LAYER_II:
        result = FLO_LayerII_DecodeFrame(frame_data, 
                                         frame_info,
                                         &self->frame.frame_II, 
                                         left_filter, 
                                         right_filter);
        break;
#endif /* FLO_CONFIG_MINI_BUILD */

      case FLO_MPEG_LAYER_III:
        result = FLO_LayerIII_DecodeFrame(frame_data, 
                                          frame_info,
                                          &self->frame.frame_III, 
                                          &self->main_data,
                                          left_filter, 
                                          right_filter);
        break;

      default:
          result = FLO_ERROR_INVALID_DECODER_STATE;
    }

    if (result == FLO_SUCCESS) {
        sample_buffer->size = sample_buffer->format.channel_count *
                              frame_info->sample_count * 2;
    } else {
        sample_buffer->size = 0;
    }

    return result;
}
#endif

/*----------------------------------------------------------------------
|   FLO_Engine_Reset
+---------------------------------------------------------------------*/
#if (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_MPG123)
FLO_Result
FLO_Engine_Reset(FLO_Engine* self)
{
    MPGLIB_Reset(&self->mpg123_decoder);
    return FLO_SUCCESS;
}
#elif (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_FFMPEG)
FLO_Result
FLO_Engine_Reset(FLO_Engine* self)
{
    avcodec_close(self->ffmpeg_decoder_context);
    free((void*)self->ffmpeg_decoder_context);
    self->ffmpeg_decoder_context = avcodec_alloc_context();
    avcodec_open(self->ffmpeg_decoder_context, 
                 self->ffmpeg_decoder_module); 
    return FLO_SUCCESS;
}
#elif (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN)
FLO_Result
FLO_Engine_Reset(FLO_Engine* self)
{
    self->main_data.available = 0;
    FLO_LayerIII_ResetFrame(&self->frame.frame_III);
    FLO_SynthesisFilter_Reset(self->left_filter);
    FLO_SynthesisFilter_Reset(self->right_filter);

    return FLO_SUCCESS;
}
#endif
