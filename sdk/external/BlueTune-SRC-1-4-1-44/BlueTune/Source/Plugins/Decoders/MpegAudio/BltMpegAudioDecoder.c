/*****************************************************************
|
|   BlueTune - Mpeg Audio Decoder Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "Fluo.h"
#include "BltConfig.h"
#include "BltCore.h"
#include "BltMpegAudioDecoder.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltPacketConsumer.h"
#include "BltStream.h"
#include "BltReplayGain.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.decoders.mpeg-audio")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_UInt32 mpeg_audio_type_id;
} MpegAudioDecoderModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketConsumer);

    /* members */
    BLT_Boolean eos;
    BLT_Boolean is_continous_stream;
    ATX_List*   packets;
} MpegAudioDecoderInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    BLT_Boolean      eos;
    BLT_PcmMediaType media_type;
    BLT_TimeStamp    time_stamp;
    ATX_Int64        sample_count;
} MpegAudioDecoderOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    BLT_Module             module;
    MpegAudioDecoderInput  input;
    MpegAudioDecoderOutput output;
    FLO_Decoder*           fluo;
    struct {
        BLT_Cardinal nominal_bitrate;
        BLT_Cardinal average_bitrate;
        ATX_Int64    average_bitrate_accumulator;
        BLT_Cardinal instant_bitrate;
        ATX_Int64    instant_bitrate_accumulator;
    }                      stream_info;
    struct {
        ATX_Flags flags;
        ATX_Int32 track_gain;
        ATX_Int32 album_gain;
    }                      replay_gain_info;
    struct {
        unsigned int level;
        unsigned int layer;
    }                      mpeg_info;
} MpegAudioDecoder;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_BITRATE_AVERAGING_SHORT_SCALE     7
#define BLT_BITRATE_AVERAGING_SHORT_WINDOW    32
#define BLT_BITRATE_AVERAGING_LONG_SCALE      7
#define BLT_BITRATE_AVERAGING_LONG_WINDOW     4096
#define BLT_BITRATE_AVERAGING_PRECISION       4000

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(MpegAudioDecoderModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(MpegAudioDecoder, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(MpegAudioDecoder, ATX_Referenceable)

/*----------------------------------------------------------------------
|   MpegAudioDecoderInput_Flush
+---------------------------------------------------------------------*/
static BLT_Result
MpegAudioDecoderInput_Flush(MpegAudioDecoder* self)
{
    ATX_ListItem* item;
    while ((item = ATX_List_GetFirstItem(self->input.packets))) {
        BLT_MediaPacket* packet = ATX_ListItem_GetData(item);
        if (packet) BLT_MediaPacket_Release(packet);
        ATX_List_RemoveItem(self->input.packets, item);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   MpegAudioDecoderInput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
MpegAudioDecoderInput_PutPacket(BLT_PacketConsumer* _self,
                                BLT_MediaPacket*    packet)
{
    MpegAudioDecoder* self = ATX_SELF_M(input, MpegAudioDecoder, BLT_PacketConsumer);
    ATX_Result        result;

    /* check to see if this is the end of a stream */
    if (BLT_MediaPacket_GetFlags(packet) & 
        BLT_MEDIA_PACKET_FLAG_END_OF_STREAM) {
        self->input.eos = BLT_TRUE;
    }

    /* add the packet to the input list */
    result = ATX_List_AddData(self->input.packets, packet);
    if (ATX_SUCCEEDED(result)) {
        BLT_MediaPacket_AddReference(packet);
    }

    return result;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(MpegAudioDecoderInput)
    ATX_GET_INTERFACE_ACCEPT(MpegAudioDecoderInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(MpegAudioDecoderInput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(MpegAudioDecoderInput, BLT_PacketConsumer)
    MpegAudioDecoderInput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(MpegAudioDecoderInput, 
                                         "input",
                                         PACKET,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(MpegAudioDecoderInput, BLT_MediaPort)
    MpegAudioDecoderInput_GetName,
    MpegAudioDecoderInput_GetProtocol,
    MpegAudioDecoderInput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   MpegAudioDecoder_UpdateInfo
+---------------------------------------------------------------------*/
static BLT_Result
MpegAudioDecoder_UpdateInfo(MpegAudioDecoder* self,     
                            FLO_FrameInfo*    frame_info)
{
    /* check if the media format has changed */
    if (frame_info->sample_rate   != self->output.media_type.sample_rate   ||
        frame_info->channel_count != self->output.media_type.channel_count ||
        frame_info->level         != self->mpeg_info.level                 ||
        frame_info->layer         != self->mpeg_info.layer) {

        if (self->output.media_type.sample_rate   != 0 ||
            self->output.media_type.channel_count != 0) {
            /* format change, discard the packet */
            ATX_LOG_FINER("MpegAudioDecoder::UpdateInfo - "
                          "format change, discarding frame");
            FLO_Decoder_SkipFrame(self->fluo);
            return FLO_ERROR_NOT_ENOUGH_DATA;
        }

        /* keep the new info */
        self->mpeg_info.layer = frame_info->layer;
        self->mpeg_info.level = frame_info->level;

        /* set the output type extensions */
        BLT_PcmMediaType_Init(&self->output.media_type);
        self->output.media_type.channel_count   = (BLT_UInt16)frame_info->channel_count;
        self->output.media_type.sample_rate     = frame_info->sample_rate;
        self->output.media_type.bits_per_sample = 16;
        self->output.media_type.sample_format   = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE;
        
        {
            BLT_StreamInfo info;
            char           data_type[32] = "MPEG-X Layer X";
            data_type[ 5] = '0' + (frame_info->level > 0 ?
                                   frame_info->level : 2);
            data_type[13] = '0' + frame_info->layer;
            info.data_type       = data_type;
            info.sample_rate     = frame_info->sample_rate;
            info.channel_count   = (BLT_UInt16)frame_info->channel_count;

            if (ATX_BASE(self, BLT_BaseMediaNode).context) {
                info.mask = 
                    BLT_STREAM_INFO_MASK_DATA_TYPE    |
                    BLT_STREAM_INFO_MASK_SAMPLE_RATE  |
                    BLT_STREAM_INFO_MASK_CHANNEL_COUNT;
                BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &info);
            }
        }
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   MpegAudioDecoder_UpdateBitrateAverage
+---------------------------------------------------------------------*/
static BLT_Cardinal
MpegAudioDecoder_UpdateBitrateAverage(BLT_Cardinal previous_bitrate,
                                      BLT_Cardinal current_bitrate,
                                      ATX_Int64*   accumulator,
                                      BLT_Cardinal window,
                                      BLT_Cardinal scale)
{
    BLT_Cardinal new_bitrate;
    long         diff_bitrate;

    if (previous_bitrate == 0) {
        *accumulator = current_bitrate << scale;
        return current_bitrate;
    }

    *accumulator *= window-1;
    *accumulator += current_bitrate << scale;
    *accumulator /= window;
    
    new_bitrate = (ATX_UInt32)(*accumulator);
    new_bitrate = (new_bitrate + (1<<(scale-1))) >> scale;
    new_bitrate = new_bitrate /
        BLT_BITRATE_AVERAGING_PRECISION *
        BLT_BITRATE_AVERAGING_PRECISION;

    /* only update if the difference is more than 1/32th of the previous one */
    diff_bitrate = new_bitrate - previous_bitrate;
    if (diff_bitrate < 0) diff_bitrate = -diff_bitrate;
    if (diff_bitrate > (long)(previous_bitrate>>5)) {
        return new_bitrate;
    } else {
        return previous_bitrate;
    }
}

/*----------------------------------------------------------------------
|   MpegAudioDecoder_UpdateDurationAndBitrate
+---------------------------------------------------------------------*/
static BLT_Result
MpegAudioDecoder_UpdateDurationAndBitrate(MpegAudioDecoder*  self,  
                                          FLO_DecoderStatus* fluo_status,
                                          FLO_FrameInfo*     frame_info)
{
    BLT_StreamInfo info;
    BLT_Result     result;
    
    if (ATX_BASE(self, BLT_BaseMediaNode).context == NULL) return BLT_SUCCESS;

    /* get the decoder status */
    result = FLO_Decoder_GetStatus(self->fluo, &fluo_status);
    if (BLT_FAILED(result)) return result;

    /* get current info */
    BLT_Stream_GetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &info);
    
    /* we always set the average bitrate */
    info.mask = BLT_STREAM_INFO_MASK_AVERAGE_BITRATE;

    /* update the duration unless this is a continuous stream */
    if ((info.flags & BLT_STREAM_INFO_FLAG_CONTINUOUS) == 0) {
        info.mask |= BLT_STREAM_INFO_MASK_DURATION;
    }
    
    /* update the info */
    if (fluo_status->flags & FLO_DECODER_STATUS_STREAM_IS_VBR) {
        info.mask |= BLT_STREAM_INFO_MASK_FLAGS;
        info.flags = BLT_STREAM_INFO_FLAG_VBR;
    }
    if (fluo_status->flags & FLO_DECODER_STATUS_STREAM_HAS_INFO) {
        /* the info was contained in the stream (VBR header) */
        info.nominal_bitrate = fluo_status->stream_info.bitrate;
        info.average_bitrate = fluo_status->stream_info.bitrate;
        info.duration        = fluo_status->stream_info.duration_ms;
        info.mask |= BLT_STREAM_INFO_MASK_NOMINAL_BITRATE;
    } else {
        /* nominal bitrate */
        if (self->stream_info.nominal_bitrate == 0) {
            info.nominal_bitrate = frame_info->bitrate;
            self->stream_info.nominal_bitrate = frame_info->bitrate;
            info.mask |= BLT_STREAM_INFO_MASK_NOMINAL_BITRATE;
        } 
        
        /* average bitrate */
        {
            info.average_bitrate = MpegAudioDecoder_UpdateBitrateAverage(
                self->stream_info.average_bitrate,
                frame_info->bitrate,
                &self->stream_info.average_bitrate_accumulator,
                BLT_BITRATE_AVERAGING_LONG_WINDOW,
                BLT_BITRATE_AVERAGING_LONG_SCALE);
            self->stream_info.average_bitrate = info.average_bitrate;
        }   

        /* compute the duration from the size and the average bitrate */
        if (info.size && info.average_bitrate) {
            ATX_UInt64 duration_ms = (8*1000*(ATX_UInt64)info.size)/info.average_bitrate;
            info.duration = duration_ms;
        } else {
            info.duration = 0;
        }
    }

    /* the instant bitrate is a short window average */
    {
        info.instant_bitrate = MpegAudioDecoder_UpdateBitrateAverage(
            self->stream_info.instant_bitrate,
            frame_info->bitrate,
            &self->stream_info.instant_bitrate_accumulator,
            BLT_BITRATE_AVERAGING_SHORT_WINDOW,
            BLT_BITRATE_AVERAGING_SHORT_SCALE);
        self->stream_info.instant_bitrate = info.instant_bitrate;
        info.mask |= BLT_STREAM_INFO_MASK_INSTANT_BITRATE;
    }

    /* update the stream info */
    BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &info);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   MpegAudioDecoder_UpdateReplayGainInfo
+---------------------------------------------------------------------*/
static void
MpegAudioDecoder_UpdateReplayGainInfo(MpegAudioDecoder*  self,  
                                      FLO_DecoderStatus* fluo_status)
{
    BLT_Boolean update = BLT_FALSE;

    if (fluo_status->flags & FLO_DECODER_STATUS_STREAM_HAS_REPLAY_GAIN) {
        /* the stream has replay gain info */
        if (self->replay_gain_info.flags != 
            fluo_status->replay_gain_info.flags) {
            /* those are new values */
            update = BLT_TRUE;

            /* set the track gain value */
            self->replay_gain_info.track_gain = 
                10 * fluo_status->replay_gain_info.track_gain;

            /* set the album gain value */
            self->replay_gain_info.album_gain = 
                10 * fluo_status->replay_gain_info.album_gain;

            /* copy the flags */
            self->replay_gain_info.flags = fluo_status->replay_gain_info.flags;
        }
    } else {
        /* the stream has no replay gain info */
        if (self->replay_gain_info.flags != 0) {
            /* we had values, clear them */
            update = BLT_TRUE;
        }
        self->replay_gain_info.flags      = 0; 
        self->replay_gain_info.album_gain = 0;
        self->replay_gain_info.track_gain = 0;
    }

    /* update the stream properties if necessary */
    if (update == BLT_TRUE) {
        ATX_Properties* properties;

        /* get a reference to the stream properties */
        if (BLT_SUCCEEDED(BLT_Stream_GetProperties(ATX_BASE(self, BLT_BaseMediaNode).context, 
                                                   &properties))) {
            ATX_PropertyValue property_value;
            property_value.type = ATX_PROPERTY_VALUE_TYPE_INTEGER;
            if (self->replay_gain_info.flags &
                FLO_REPLAY_GAIN_HAS_TRACK_VALUE) {
                property_value.data.integer = self->replay_gain_info.track_gain;
                ATX_Properties_SetProperty(properties,
                                           BLT_REPLAY_GAIN_TRACK_GAIN_VALUE,
                                           &property_value);
            } else {
                ATX_Properties_SetProperty(properties,
                                             BLT_REPLAY_GAIN_TRACK_GAIN_VALUE,
                                             NULL);
            }
            if (self->replay_gain_info.flags &
                FLO_REPLAY_GAIN_HAS_ALBUM_VALUE) {
                property_value.data.integer = self->replay_gain_info.album_gain;
                ATX_Properties_SetProperty(properties,
                                           BLT_REPLAY_GAIN_ALBUM_GAIN_VALUE,
                                           &property_value);
            } else {
                ATX_Properties_SetProperty(properties,
                                             BLT_REPLAY_GAIN_ALBUM_GAIN_VALUE,
                                             NULL);
            }
        }
    }
}

/*----------------------------------------------------------------------
|   MpegAudioDecoder_DecodeFrame
+---------------------------------------------------------------------*/
static BLT_Result
MpegAudioDecoder_DecodeFrame(MpegAudioDecoder* self,
                             BLT_MediaPacket** packet)
{
    FLO_SampleBuffer   sample_buffer;
    FLO_FrameInfo      frame_info;        
    FLO_DecoderStatus* fluo_status;
    FLO_Cardinal       samples_skipped = 0;
    FLO_Result         result;
    
    /* try to find a frame */
    result = FLO_Decoder_FindFrame(self->fluo, &frame_info);
    if (FLO_FAILED(result)) return result;

    /* setup default return value */
    *packet = NULL;

    /* update the stream info */
    result = MpegAudioDecoder_UpdateInfo(self, &frame_info);
    if (BLT_FAILED(result)) return result;

    /* get the decoder status */
    result = FLO_Decoder_GetStatus(self->fluo, &fluo_status);
    if (BLT_FAILED(result)) return result;

    /* update the bitrate */
    result = MpegAudioDecoder_UpdateDurationAndBitrate(self, fluo_status, &frame_info);
    if (BLT_FAILED(result)) return result;

    /* update the replay gain info */
    MpegAudioDecoder_UpdateReplayGainInfo(self, fluo_status);

    /* get a packet from the core */
    sample_buffer.size = frame_info.sample_count*frame_info.channel_count*2;
    result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
                                        sample_buffer.size,
                                        (const BLT_MediaType*)&self->output.media_type,
                                        packet);
    if (BLT_FAILED(result)) return result;

    /* get the address of the packet payload */
    sample_buffer.samples = BLT_MediaPacket_GetPayloadBuffer(*packet);

    /* decode the frame */
    result = FLO_Decoder_DecodeFrame(self->fluo, 
                                     &sample_buffer,
                                     &samples_skipped);
    if (FLO_FAILED(result)) {
        /* check fluo result */
        if (result == FLO_ERROR_NO_MORE_SAMPLES) {
            /* we have already decoded everything in the stream, but there     */
            /* could be more coming if the input is a sequence of concatenated */
            /* streams, such as in a streaming scenario                        */ 
            FLO_Decoder_Reset(self->fluo, FLO_TRUE);
        }

        /* release the packet */
        BLT_MediaPacket_Release(*packet);
        *packet = NULL;
        return result;
    }

    /* adjust for skipped samples */
    if (samples_skipped) {
        BLT_Offset offset = samples_skipped*sample_buffer.format.channel_count*2;
        BLT_MediaPacket_SetPayloadWindow(*packet, offset, sample_buffer.size);
    } else {
        /* set the packet payload size */
        BLT_MediaPacket_SetPayloadSize(*packet, sample_buffer.size);
    }

    /* update the sample count */
    self->output.sample_count = fluo_status->sample_count;

    /* set start of stream packet flags */
    {
        if (self->output.sample_count == 0) {
            BLT_MediaPacket_SetFlags(*packet, 
                                     BLT_MEDIA_PACKET_FLAG_START_OF_STREAM);
        }
    }

    /* update the timestamp */
    if (frame_info.channel_count             != 0 && 
        frame_info.sample_rate               != 0 &&
        sample_buffer.format.bits_per_sample != 0) {
        /* compute time stamp */
        self->output.time_stamp = 
            BLT_TimeStamp_FromSamples(self->output.sample_count,
                                      frame_info.sample_rate);
        BLT_MediaPacket_SetTimeStamp(*packet, self->output.time_stamp);
    } 

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   MpegAudioDecoderOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
MpegAudioDecoderOutput_GetPacket(BLT_PacketProducer* _self,
                                 BLT_MediaPacket**   packet)
{
    MpegAudioDecoder* self = ATX_SELF_M(output, MpegAudioDecoder, BLT_PacketProducer);
    ATX_ListItem*     item;
    BLT_Any           payload_buffer;
    BLT_Size          payload_size;
    BLT_Boolean       try_again;
    BLT_Result        result;

    /* default return */
    *packet = NULL;

    /* check for EOS */
    if (self->output.eos) {
        return BLT_ERROR_EOS;
    }

    do {
        /* try to decode a frame */
        result = MpegAudioDecoder_DecodeFrame(self, packet);
        if (BLT_SUCCEEDED(result)) return BLT_SUCCESS;
        if (FLO_ERROR_IS_FATAL(result)) {
            return result;
        }             

        /* not enough data, try to feed some more */
        try_again = BLT_FALSE;
        if ((item = ATX_List_GetFirstItem(self->input.packets))) {
            BLT_MediaPacket* input = ATX_ListItem_GetData(item);
            BLT_Size         feed_size;
            FLO_Flags        flags = 0;

            /* get the packet payload */
            payload_buffer = BLT_MediaPacket_GetPayloadBuffer(input);
            payload_size   = BLT_MediaPacket_GetPayloadSize(input);

            /* compute the flags */
            if (ATX_List_GetItemCount(self->input.packets) == 0) {
                /* no more packets */
                if (self->input.eos) {
                    /* end of stream */
                    flags |= FLO_DECODER_BUFFER_IS_END_OF_STREAM;
                }
            }
            if (BLT_MediaPacket_GetFlags(input) & 
                BLT_MEDIA_PACKET_FLAG_END_OF_STREAM) {
                flags |= FLO_DECODER_BUFFER_IS_END_OF_STREAM;
            }

            /* feed the decoder */
            feed_size = payload_size;
            result = FLO_Decoder_Feed(self->fluo, 
                                      payload_buffer, 
                                      &feed_size, flags);
            if (BLT_FAILED(result)) return result;

            if (feed_size == payload_size) {
                /* we're done with the packet */
                ATX_List_RemoveItem(self->input.packets, item);
                BLT_MediaPacket_Release(input);
            } else {
                /* we can't feed anymore, there's some leftovers */
                BLT_Offset offset = BLT_MediaPacket_GetPayloadOffset(input);
                BLT_MediaPacket_SetPayloadOffset(input, offset+feed_size);
            }
            if (feed_size != 0 || flags) {
                try_again = BLT_TRUE;
            }
        } 
    } while (try_again);

    /* if we've reached the end of stream, generate an empty packet with */
    /* a flag to indicate that situation                                 */
    if (self->input.eos) {
        result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
                                            0,
                                            (const BLT_MediaType*)&self->output.media_type,
                                            packet);
        if (BLT_FAILED(result)) return result;
        BLT_MediaPacket_SetFlags(*packet, BLT_MEDIA_PACKET_FLAG_END_OF_STREAM);
        BLT_MediaPacket_SetTimeStamp(*packet, self->output.time_stamp);
        self->output.eos = BLT_TRUE;
        return BLT_SUCCESS;
    }
    
    return BLT_ERROR_PORT_HAS_NO_DATA;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(MpegAudioDecoderOutput)
    ATX_GET_INTERFACE_ACCEPT(MpegAudioDecoderOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(MpegAudioDecoderOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(MpegAudioDecoderOutput, 
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(MpegAudioDecoderOutput, BLT_MediaPort)
    MpegAudioDecoderOutput_GetName,
    MpegAudioDecoderOutput_GetProtocol,
    MpegAudioDecoderOutput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(MpegAudioDecoderOutput, BLT_PacketProducer)
    MpegAudioDecoderOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   MpegAudioDecoder_SetupPorts
+---------------------------------------------------------------------*/
static BLT_Result
MpegAudioDecoder_SetupPorts(MpegAudioDecoder* self)
{
    ATX_Result result;

    /* init the input port */
    self->input.eos = BLT_FALSE;

    /* create a list of input packets */
    result = ATX_List_Create(&self->input.packets);
    if (ATX_FAILED(result)) return result;
    
    /* setup the output port */
    self->output.eos = BLT_FALSE;
    BLT_PcmMediaType_Init(&self->output.media_type);
    self->output.sample_count = 0;
    BLT_TimeStamp_Set(self->output.time_stamp, 0, 0);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    MpegAudioDecoder_Create
+---------------------------------------------------------------------*/
static BLT_Result
MpegAudioDecoder_Create(BLT_Module*              module,
                        BLT_Core*                core, 
                        BLT_ModuleParametersType parameters_type,
                        BLT_CString              parameters, 
                        BLT_MediaNode**          object)
{
    MpegAudioDecoder* self;
    BLT_Result        result;

    ATX_LOG_FINE("MpegAudioDecoder::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(MpegAudioDecoder));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* create the fluo decoder */
    result = FLO_Decoder_Create(&self->fluo);
    if (FLO_FAILED(result)) {
        ATX_FreeMemory(self);
        *object = NULL;
        return result;
    }

    /* setup the input and output ports */
    result = MpegAudioDecoder_SetupPorts(self);
    if (BLT_FAILED(result)) {
        ATX_FreeMemory(self);
        *object = NULL;
        return result;
    }

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, MpegAudioDecoder, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, MpegAudioDecoder, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  MpegAudioDecoderInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  MpegAudioDecoderInput,  BLT_PacketConsumer);
    ATX_SET_INTERFACE(&self->output, MpegAudioDecoderOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, MpegAudioDecoderOutput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    MpegAudioDecoder_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
MpegAudioDecoder_Destroy(MpegAudioDecoder* self)
{ 
    ATX_ListItem* item;

    ATX_LOG_FINE("MpegAudioDecoder::Destroy");

    /* release any packet we may hold */
    item = ATX_List_GetFirstItem(self->input.packets);
    while (item) {
        BLT_MediaPacket* packet = ATX_ListItem_GetData(item);
        if (packet) {
            BLT_MediaPacket_Release(packet);
        }
        item = ATX_ListItem_GetNext(item);
    }
    ATX_List_Destroy(self->input.packets);
    
    /* destroy the fluo decoder */
    FLO_Decoder_Destroy(self->fluo);
    
    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}
                    
/*----------------------------------------------------------------------
|   MpegAudioDecoder_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
MpegAudioDecoder_GetPortByName(BLT_MediaNode*  _self,
                               BLT_CString     name,
                               BLT_MediaPort** port)
{
    MpegAudioDecoder* self = ATX_SELF_EX(MpegAudioDecoder, BLT_BaseMediaNode, BLT_MediaNode);

    if (ATX_StringsEqual(name, "input")) {
        *port = &ATX_BASE(&self->input, BLT_MediaPort);
        return BLT_SUCCESS;
    } else if (ATX_StringsEqual(name, "output")) {
        *port = &ATX_BASE(&self->output, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|    MpegAudioDecoder_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
MpegAudioDecoder_Seek(BLT_MediaNode* _self,
                      BLT_SeekMode*  mode,
                      BLT_SeekPoint* point)
{
    MpegAudioDecoder* self = ATX_SELF_EX(MpegAudioDecoder, BLT_BaseMediaNode, BLT_MediaNode);

    /* flush pending input packets */
    MpegAudioDecoderInput_Flush(self);

    /* clear the eos flag */
    self->input.eos  = BLT_FALSE;
    self->output.eos = BLT_FALSE;

    /* flush and reset the decoder */
    FLO_Decoder_Flush(self->fluo);
    FLO_Decoder_Reset(self->fluo, FLO_FALSE);

    /* estimate the seek point in time_stamp mode */
    if (ATX_BASE(self, BLT_BaseMediaNode).context == NULL) return BLT_FAILURE;
    BLT_Stream_EstimateSeekPoint(ATX_BASE(self, BLT_BaseMediaNode).context, *mode, point);
    if (!(point->mask & BLT_SEEK_POINT_MASK_SAMPLE)) {
        return BLT_FAILURE;
    }

    /* update the decoder's sample position */
    self->output.sample_count = point->sample;
    self->output.time_stamp = point->time_stamp;
    FLO_Decoder_SetSample(self->fluo, point->sample);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(MpegAudioDecoder)
    ATX_GET_INTERFACE_ACCEPT_EX(MpegAudioDecoder, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(MpegAudioDecoder, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(MpegAudioDecoder, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    MpegAudioDecoder_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    MpegAudioDecoder_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(MpegAudioDecoder, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   MpegAudioDecoderModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
MpegAudioDecoderModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    MpegAudioDecoderModule* self = ATX_SELF_EX(MpegAudioDecoderModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*           registry;
    BLT_Result              result;

    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the .mp2, .mp1, .mp3 .mpa and .mpg file extensions */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".mp3",
                                            "audio/mpeg");
    if (BLT_FAILED(result)) return result;
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".mp2",
                                            "audio/mpeg");
    if (BLT_FAILED(result)) return result;
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".mp1",
                                            "audio/mpeg");
    if (BLT_FAILED(result)) return result;
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".mpa",
                                            "audio/mpeg");
    if (BLT_FAILED(result)) return result;
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".mpg",
                                            "audio/mpeg");
    if (BLT_FAILED(result)) return result;

    /* register the "audio/mpeg" type */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/mpeg",
        &self->mpeg_audio_type_id);
    if (BLT_FAILED(result)) return result;
    
    /* register mime type aliases */
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/mp3", self->mpeg_audio_type_id);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/x-mp3", self->mpeg_audio_type_id);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/mpg", self->mpeg_audio_type_id);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/x-mpg", self->mpeg_audio_type_id);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/x-mpeg", self->mpeg_audio_type_id);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/mpeg3", self->mpeg_audio_type_id);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/x-mpeg3", self->mpeg_audio_type_id);

    ATX_LOG_FINE_1("MpegAudioDecoderModule::Attach (audio/mpeg type = %d)", self->mpeg_audio_type_id);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   MpegAudioDecoderModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
MpegAudioDecoderModule_Probe(BLT_Module*              _self, 
                             BLT_Core*                core,
                             BLT_ModuleParametersType parameters_type,
                             BLT_AnyConst             parameters,
                             BLT_Cardinal*            match)
{
    MpegAudioDecoderModule* self = ATX_SELF_EX(MpegAudioDecoderModule, BLT_BaseModule, BLT_Module);
    BLT_COMPILER_UNUSED(core);
    
    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

            /* the input and output protocols should be PACKET */
            if ((constructor->spec.input.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_PACKET) ||
                (constructor->spec.output.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_PACKET)) {
                return BLT_FAILURE;
            }

            /* the input type should be audio/mpeg */
            if (constructor->spec.input.media_type->id != 
                self->mpeg_audio_type_id) {
                return BLT_FAILURE;
            }

            /* the output type should be unspecified, or audio/pcm */
            if (!(constructor->spec.output.media_type->id == 
                  BLT_MEDIA_TYPE_ID_AUDIO_PCM) &&
                !(constructor->spec.output.media_type->id ==
                  BLT_MEDIA_TYPE_ID_UNKNOWN)) {
                return BLT_FAILURE;
            }

            /* compute the match level */
            if (constructor->name != NULL) {
                /* we're being probed by name */
                if (ATX_StringsEqual(constructor->name, "MpegAudioDecoder")) {
                    /* our name */
                    *match = BLT_MODULE_PROBE_MATCH_EXACT;
                } else {
                    /* not our name */
                    return BLT_FAILURE;
                }
            } else {
                /* we're probed by protocol/type specs only */
                *match = BLT_MODULE_PROBE_MATCH_MAX - 10;
            }

            ATX_LOG_FINE_1("MpegAudioDecoderModule::Probe - Ok [%d]", *match);
            return BLT_SUCCESS;
        }    
        break;

      default:
        break;
    }

    return BLT_FAILURE;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(MpegAudioDecoderModule)
    ATX_GET_INTERFACE_ACCEPT_EX(MpegAudioDecoderModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(MpegAudioDecoderModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(MpegAudioDecoderModule, MpegAudioDecoder)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(MpegAudioDecoderModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    MpegAudioDecoderModule_Attach,
    MpegAudioDecoderModule_CreateInstance,
    MpegAudioDecoderModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define MpegAudioDecoderModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(MpegAudioDecoderModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(MpegAudioDecoderModule,
                                         "MPEG Audio Decoder",
                                         "com.axiosys.decoder.mpeg-audio",
                                         "1.4.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
