/****************************************************************
|
|   Vorbis Decoder Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltVorbisDecoder.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltByteStreamUser.h"
#include "BltStream.h"
#include "BltReplayGain.h"

#if defined(BLT_CONFIG_VORBIS_USE_TREMOR)
#include "ivorbisfile.h"
#include "ivorbiscodec.h"
#else
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"
#endif

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.decoders.vorbis")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_UInt32 ogg_type_id;
} VorbisDecoderModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamUser);

    /* members */
    BLT_Boolean      eos;
    ATX_InputStream* stream;
    BLT_LargeSize    size;
    BLT_MediaTypeId  media_type_id;
    OggVorbis_File   vorbis_file;
} VorbisDecoderInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    BLT_PcmMediaType media_type;
    BLT_Cardinal     packet_count;
    ATX_Int64        sample_count;
} VorbisDecoderOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    VorbisDecoderInput  input;
    VorbisDecoderOutput output;
} VorbisDecoder;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_VORBIS_DECODER_PACKET_SIZE 4096

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(VorbisDecoderModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(VorbisDecoder, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(VorbisDecoder, ATX_Referenceable)

/*----------------------------------------------------------------------
|   VorbisDecoder_ReadCallback
+---------------------------------------------------------------------*/
static size_t
VorbisDecoder_ReadCallback(void *ptr, size_t size, size_t nbelem, void *datasource)
{
    VorbisDecoder* self = (VorbisDecoder*)datasource;
    BLT_Size       bytes_to_read;
    BLT_Size       bytes_read;
    BLT_Result     result;

    bytes_to_read = (BLT_Size)size*nbelem;
    result = ATX_InputStream_Read(self->input.stream, ptr, bytes_to_read, &bytes_read);
    if (BLT_SUCCEEDED(result)) {
        return bytes_read;
    } else if (result == BLT_ERROR_EOS) {
        return 0;
    } else {
        return -1;
    }
}

/*----------------------------------------------------------------------
|   VorbisDecoder_SeekCallback
+---------------------------------------------------------------------*/
static int
VorbisDecoder_SeekCallback(void *datasource, ogg_int64_t offset, int whence)
{
    VorbisDecoder* self = (VorbisDecoder *)datasource;
    BLT_Position   where;
    BLT_Result     result;

    /* compute where to seek */
    if (whence == SEEK_CUR) {
        /* special case for offset == 0, vorbis uses that to test */
        /* wether the input can seek or not.                      */
        if (offset == 0 && self->input.size == 0) {
            /* most likely an unseekable network stream */
            return -1;
        } else {
            ATX_Position current;
            ATX_InputStream_Tell(self->input.stream, &current);
            if (current+offset <= self->input.size) {
                where = current+(long)offset;
            } else {
                where = self->input.size;
            }
        }
    } else if (whence == SEEK_END) {
        if (offset <= (ogg_int64_t)self->input.size) {
            where = self->input.size - offset;
        } else {
            where = 0;
        }
    } else if (whence == SEEK_SET) {
        where = (long)offset;
    } else {
        return -1;
    }

    /* clear the eos flag */
    self->input.eos = BLT_FALSE;

    /* perform the seek */
    result = ATX_InputStream_Seek(self->input.stream, where);
    if (BLT_FAILED(result)) {
        return -1;
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------
|   VorbisDecoder_CloseCallback
+---------------------------------------------------------------------*/
static int
VorbisDecoder_CloseCallback(void *datasource)
{
    /* ignore */
    BLT_COMPILER_UNUSED(datasource);
    return 0;
}

/*----------------------------------------------------------------------
|   VorbisDecoder_TellCallback
+---------------------------------------------------------------------*/
static long
VorbisDecoder_TellCallback(void *datasource)
{
    VorbisDecoder *self = (VorbisDecoder *)datasource;
    ATX_Position   position;
    BLT_Result     result;

    result = ATX_InputStream_Tell(self->input.stream, &position);
    if (BLT_SUCCEEDED(result)) {
        return (long)position;
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------
|   VorbisDecoder_OpenStream
+---------------------------------------------------------------------*/
BLT_METHOD
VorbisDecoder_OpenStream(VorbisDecoder* self)
{
    ov_callbacks    callbacks;
    vorbis_info*    info;
    vorbis_comment* comment;
    int             result;
    
    /* check that we have a stream */
    if (self->input.stream == NULL) {
        return BLT_FAILURE;
    }

    /* clear the eos flag */
    self->input.eos = BLT_FALSE;

    /* get input stream size */
    ATX_InputStream_GetSize(self->input.stream, &self->input.size);

    /* setup callbacks */
    callbacks.read_func  = VorbisDecoder_ReadCallback;
    callbacks.seek_func  = VorbisDecoder_SeekCallback;
    callbacks.close_func = VorbisDecoder_CloseCallback;
    callbacks.tell_func  = VorbisDecoder_TellCallback;

    /* initialize the vorbis file structure */
    result = ov_open_callbacks(self, 
                               &self->input.vorbis_file, 
                               NULL, 
                               0, 
                               callbacks);
    if (result < 0) {
        self->input.vorbis_file.dataoffsets = NULL;
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }

    /* get info about the stream */
    info = ov_info(&self->input.vorbis_file, -1);
    if (info == NULL) return BLT_ERROR_INVALID_MEDIA_FORMAT;
    self->output.media_type.sample_rate     = info->rate;
    self->output.media_type.channel_count   = info->channels;
    self->output.media_type.channel_mask    = 0;
    self->output.media_type.bits_per_sample = 16;
    self->output.media_type.sample_format   = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE;

    /* update the stream info */
    if (ATX_BASE(self, BLT_BaseMediaNode).context) {
        BLT_StreamInfo stream_info;

        /* start with no info */
        stream_info.mask = 0;

        /* sample rate */
        stream_info.sample_rate = info->rate;
        stream_info.mask |= BLT_STREAM_INFO_MASK_SAMPLE_RATE;

        /* channel count */
        stream_info.channel_count = info->channels;
        stream_info.mask |= BLT_STREAM_INFO_MASK_CHANNEL_COUNT;

        /* data type */
        stream_info.data_type = "Vorbis";
        stream_info.mask |= BLT_STREAM_INFO_MASK_DATA_TYPE;

        /* nominal bitrate */
        stream_info.nominal_bitrate = info->bitrate_nominal;
        stream_info.mask |= BLT_STREAM_INFO_MASK_NOMINAL_BITRATE;

        /* average bitrate */
        {
            long bitrate = ov_bitrate(&self->input.vorbis_file, -1);
            if (bitrate > 0) {
                stream_info.average_bitrate = bitrate;
            } else {
                stream_info.average_bitrate = info->bitrate_nominal;
            }
            stream_info.mask |= BLT_STREAM_INFO_MASK_AVERAGE_BITRATE;
        }
        
        /* instant bitrate (not computed for now) */
        stream_info.instant_bitrate = stream_info.average_bitrate;
        
        /* duration */
        stream_info.duration = 0;
        if (info->rate) {
            ogg_int64_t ogg_duration = ov_pcm_total(&self->input.vorbis_file,-1);
            if (ogg_duration > 0) {
                stream_info.duration = (long)(1000.0f*(float)ogg_duration/(float)info->rate);
                stream_info.mask |= BLT_STREAM_INFO_MASK_DURATION;
            } else {
                /* try to estimate the duration from the stream size */
                if (self->input.size && stream_info.average_bitrate) {
                    stream_info.duration = (long)(1000.0f*(float)self->input.size/(float)stream_info.average_bitrate);
                    stream_info.mask |= BLT_STREAM_INFO_MASK_DURATION;
                }
            }
        }   

        BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &stream_info);
    }

    /* process the comments */
    comment = ov_comment(&self->input.vorbis_file, -1);
    if (comment) {
        int i;
        ATX_String   string = ATX_EMPTY_STRING;
        ATX_String   key    = ATX_EMPTY_STRING;
        ATX_String   value  = ATX_EMPTY_STRING;
        float        track_gain = 0.0f;
        float        album_gain = 0.0f;
        BLT_ReplayGainSetMode track_gain_mode = BLT_REPLAY_GAIN_SET_MODE_IGNORE;
        BLT_ReplayGainSetMode album_gain_mode = BLT_REPLAY_GAIN_SET_MODE_IGNORE;

        for (i=0; i<comment->comments; i++) {
            int sep;
            ATX_String_AssignN(&string, 
                               comment->user_comments[i],
                               comment->comment_lengths[i]);
            sep = ATX_String_FindChar(&string, '=');
            if (sep == ATX_STRING_SEARCH_FAILED) continue;
            ATX_String_AssignN(&key, ATX_CSTR(string), sep);
            ATX_String_Assign(&value, ATX_CSTR(string)+sep+1);

            ATX_LOG_FINE_3("  COMMENT %d : %s = %s", i, ATX_CSTR(key), ATX_CSTR(value));
            ATX_String_MakeUppercase(&key);
            if (ATX_String_Equals(&key, BLT_VORBIS_COMMENT_REPLAY_GAIN_TRACK_GAIN, ATX_FALSE)) {
                ATX_String_ToFloat(&value, &track_gain, ATX_TRUE);
                track_gain_mode = BLT_REPLAY_GAIN_SET_MODE_UPDATE;
            } else if (ATX_String_Equals(&key, BLT_VORBIS_COMMENT_REPLAY_GAIN_ALBUM_GAIN, ATX_FALSE)) {
                ATX_String_ToFloat(&value, &album_gain, ATX_TRUE);
                album_gain_mode = BLT_REPLAY_GAIN_SET_MODE_UPDATE;
            }
        }

        /* update the stream info */
        BLT_ReplayGain_SetStreamProperties(ATX_BASE(self, BLT_BaseMediaNode).context,
                                           track_gain, track_gain_mode,
                                           album_gain, album_gain_mode);

        ATX_String_Destruct(&string);
        ATX_String_Destruct(&key);
        ATX_String_Destruct(&value);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   VorbisDecoderInput_SetStream
+---------------------------------------------------------------------*/
static BLT_Result
VorbisDecoderInput_SetStream(BLT_InputStreamUser* _self, 
                             ATX_InputStream*     stream,
                             const BLT_MediaType* media_type)
{
    VorbisDecoder* self = ATX_SELF_M(input, VorbisDecoder, BLT_InputStreamUser);
    BLT_Result     result;

    /* check the stream's media type */
    if (media_type == NULL || 
        media_type->id != self->input.media_type_id) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* if we had a stream, release it */
    ATX_RELEASE_OBJECT(self->input.stream);

    /* reset counters and flags */
    self->input.size = 0;
    self->input.eos  = BLT_FALSE;
    self->output.packet_count = 0;
    self->output.sample_count = 0;

    /* open the stream */
    self->input.stream = stream;
    result = VorbisDecoder_OpenStream(self);
    if (BLT_FAILED(result)) {
        self->input.stream = NULL;
        ATX_LOG_WARNING("VorbisDecoderInput::SetStream - failed");
        return result;
    }

    /* keep a reference to the stream */
    ATX_REFERENCE_OBJECT(stream);

    /* get stream size */
    ATX_InputStream_GetSize(stream, &self->input.size);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(VorbisDecoderInput)
    ATX_GET_INTERFACE_ACCEPT(VorbisDecoderInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(VorbisDecoderInput, BLT_InputStreamUser)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_InputStreamUser interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(VorbisDecoderInput, BLT_InputStreamUser)
    VorbisDecoderInput_SetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(VorbisDecoderInput,
                                         "input",
                                         STREAM_PULL,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(VorbisDecoderInput, BLT_MediaPort)
    VorbisDecoderInput_GetName,
    VorbisDecoderInput_GetProtocol,
    VorbisDecoderInput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    VorbisDecoderOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
VorbisDecoderOutput_GetPacket(BLT_PacketProducer* _self,
                              BLT_MediaPacket**   packet)
{
    VorbisDecoder* self = ATX_SELF_M(output, VorbisDecoder, BLT_PacketProducer);
    BLT_Any        buffer;
    int            current_section;
    long           bytes_read;
    BLT_Result     result;
    
    /* check for EOS */
    if (self->input.eos) {
        *packet = NULL;
        return BLT_ERROR_EOS;
    }

    /* get a packet from the core */
    result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
                                        BLT_VORBIS_DECODER_PACKET_SIZE,
                                        (BLT_MediaType*)&self->output.media_type,
                                        packet);
    if (BLT_FAILED(result)) return result;

    /* get the addr of the buffer */
    buffer = BLT_MediaPacket_GetPayloadBuffer(*packet);

    /* decode some audio samples */
    do {
#if defined(BLT_CONFIG_VORBIS_USE_TREMOR)
        bytes_read = ov_read(&self->input.vorbis_file,
                             buffer,
                             BLT_VORBIS_DECODER_PACKET_SIZE,
                             &current_section);
#else
        bytes_read = ov_read(&self->input.vorbis_file,
                             buffer,
                             BLT_VORBIS_DECODER_PACKET_SIZE,
                             0, 2, 1, &current_section);
#endif
    } while (bytes_read == OV_HOLE);
    if (bytes_read == 0) {
        self->input.eos = BLT_TRUE;
        BLT_MediaPacket_SetFlags(*packet, 
                                 BLT_MEDIA_PACKET_FLAG_END_OF_STREAM);    
    } else if (bytes_read < 0) {
        *packet = NULL;
        BLT_MediaPacket_Release(*packet);
        return BLT_FAILURE;
    }   
    
    /* update the size of the packet */
    BLT_MediaPacket_SetPayloadSize(*packet, bytes_read);

    /* set flags */     
    if (self->output.packet_count == 0) {
        /* this is the first packet */
        BLT_MediaPacket_SetFlags(*packet,
                                 BLT_MEDIA_PACKET_FLAG_START_OF_STREAM);
    }

    /* update the sample count and timestamp */
    if (self->output.media_type.channel_count   != 0 && 
        self->output.media_type.bits_per_sample != 0 &&
        self->output.media_type.sample_rate     != 0) {
        BLT_UInt32 sample_count;

            /* compute time stamp */
        BLT_TimeStamp time_stamp = BLT_TimeStamp_FromSamples( 
                                  self->output.sample_count,
                                  self->output.media_type.sample_rate);
        BLT_MediaPacket_SetTimeStamp(*packet, time_stamp);

        /* update sample count */
        sample_count = bytes_read/(self->output.media_type.channel_count*
                                   self->output.media_type.bits_per_sample/8);
        self->output.sample_count += sample_count;
    } 

    /* update the packet count */
    self->output.packet_count++;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(VorbisDecoderOutput)
    ATX_GET_INTERFACE_ACCEPT(VorbisDecoderOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(VorbisDecoderOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(VorbisDecoderOutput,
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(VorbisDecoderOutput, BLT_MediaPort)
    VorbisDecoderOutput_GetName,
    VorbisDecoderOutput_GetProtocol,
    VorbisDecoderOutput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(VorbisDecoderOutput, BLT_PacketProducer)
    VorbisDecoderOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    VorbisDecoder_Create
+---------------------------------------------------------------------*/
static BLT_Result
VorbisDecoder_Create(BLT_Module*              module,
                     BLT_Core*                core, 
                     BLT_ModuleParametersType parameters_type,
                     BLT_CString              parameters, 
                     BLT_MediaNode**          object)
{
    VorbisDecoder* decoder;

    ATX_LOG_FINE("VorbisDecoder::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    decoder = ATX_AllocateZeroMemory(sizeof(VorbisDecoder));
    if (decoder == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(decoder, BLT_BaseMediaNode), module, core);

    /* construct the object */
    decoder->input.media_type_id = ATX_SELF_EX_O(module, VorbisDecoderModule, BLT_BaseModule, BLT_Module)->ogg_type_id;
    BLT_PcmMediaType_Init(&decoder->output.media_type);

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(decoder, VorbisDecoder, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(decoder, VorbisDecoder, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&decoder->input,  VorbisDecoderInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&decoder->input,  VorbisDecoderInput,  BLT_InputStreamUser);
    ATX_SET_INTERFACE(&decoder->output, VorbisDecoderOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&decoder->output, VorbisDecoderOutput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(decoder, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    VorbisDecoder_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
VorbisDecoder_Destroy(VorbisDecoder* self)
{
    ATX_LOG_FINE("VorbisDecoder::Destroy");

    /* release the input stream */
    ATX_RELEASE_OBJECT(self->input.stream);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory((void*)self);

    return BLT_SUCCESS;
}
                    
/*----------------------------------------------------------------------
|    VorbisDecoder_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
VorbisDecoder_Deactivate(BLT_MediaNode* _self)
{
    VorbisDecoder* self = ATX_SELF_EX(VorbisDecoder, BLT_BaseMediaNode, BLT_MediaNode);

    ATX_LOG_FINER("VorbisDecoder::Deactivate");

    /* free the vorbis decoder */
    if (self->input.stream) {
        ov_clear(&self->input.vorbis_file);

        /* release the input stream */
        ATX_RELEASE_OBJECT(self->input.stream);
    }

    /* call the base class method */
    BLT_BaseMediaNode_Deactivate(_self);

    return BLT_SUCCESS;
}
                    
/*----------------------------------------------------------------------
|   VorbisDecoder_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
VorbisDecoder_GetPortByName(BLT_MediaNode*  _self,
                            BLT_CString     name,
                            BLT_MediaPort** port)
{
    VorbisDecoder* self = ATX_SELF_EX(VorbisDecoder, BLT_BaseMediaNode, BLT_MediaNode);

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
|    VorbisDecoder_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
VorbisDecoder_Seek(BLT_MediaNode* _self,
                   BLT_SeekMode*  mode,
                   BLT_SeekPoint* point)
{
    VorbisDecoder* self = ATX_SELF_EX(VorbisDecoder, BLT_BaseMediaNode, BLT_MediaNode);
    int            ov_result;

    /* estimate the seek point in time_stamp mode */
    if (ATX_BASE(self, BLT_BaseMediaNode).context == NULL) return BLT_FAILURE;
    BLT_Stream_EstimateSeekPoint(ATX_BASE(self, BLT_BaseMediaNode).context, *mode, point);
    if (!(point->mask & BLT_SEEK_POINT_MASK_TIME_STAMP) ||
        !(point->mask & BLT_SEEK_POINT_MASK_SAMPLE)) {
        return BLT_FAILURE;
    }

    /* update the output sample count */
    self->output.sample_count = point->sample;

    /* seek to the target time */
    {
#if defined(BLT_CONFIG_VORBIS_USE_TREMOR)
        ogg_int64_t time = 
            (ogg_int64_t)point->time_stamp.seconds*1000 +
            (ogg_int64_t)point->time_stamp.nanoseconds/1000000;
        ATX_LOG_FINER_1("VorbisDecoder::Seek - sample = %ld", (long)time);
#else
        double time = 
            (double)point->time_stamp.seconds +
            (double)point->time_stamp.nanoseconds/1000000000.0f;
        ATX_LOG_FINER_1("VorbisDecoder::Seek - sample = %f", time);
#endif
        ov_result = ov_time_seek(&self->input.vorbis_file, time);
        if (ov_result != 0) return BLT_FAILURE;
    }

    /* set the mode so that the nodes down the chain know the seek has */
    /* already been done on the stream                                 */
    *mode = BLT_SEEK_MODE_IGNORE;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(VorbisDecoder)
    ATX_GET_INTERFACE_ACCEPT_EX(VorbisDecoder, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(VorbisDecoder, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(VorbisDecoder, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    VorbisDecoder_GetPortByName,
    BLT_BaseMediaNode_Activate,
    VorbisDecoder_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    VorbisDecoder_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(VorbisDecoder, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   VorbisDecoderModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
VorbisDecoderModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    VorbisDecoderModule* self = ATX_SELF_EX(VorbisDecoderModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*        registry;
    BLT_Result           result;

    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the ".ogg" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".ogg",
                                            "application/ogg");
    if (BLT_FAILED(result)) return result;

    /* register the "application/x-ogg" type */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "application/ogg",
        &self->ogg_type_id);
    if (BLT_FAILED(result)) return result;
    
    ATX_LOG_FINE_1("VorbisDecoderModule::Attach (application/ogg type = %d)", self->ogg_type_id);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   VorbisDecoderModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
VorbisDecoderModule_Probe(BLT_Module*              _self, 
                          BLT_Core*                core,
                          BLT_ModuleParametersType parameters_type,
                          BLT_AnyConst             parameters,
                          BLT_Cardinal*            match)
{
    VorbisDecoderModule* self = ATX_SELF_EX(VorbisDecoderModule, BLT_BaseModule, BLT_Module);
    BLT_COMPILER_UNUSED(core);

    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

            /* the input protocol should be STREAM_PULL and the */
            /* output protocol should be PACKET                 */
            if ((constructor->spec.input.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL) ||
                (constructor->spec.output.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_PACKET)) {
                return BLT_FAILURE;
            }

            /* the input type should be audio/x-ogg */
            if (constructor->spec.input.media_type->id != 
                self->ogg_type_id) {
                return BLT_FAILURE;
            }

            /* the output type should be unspecified, or audio/pcm */
            if (!(constructor->spec.output.media_type->id == 
                  BLT_MEDIA_TYPE_ID_UNKNOWN) &&
                !(constructor->spec.output.media_type->id == 
                  BLT_MEDIA_TYPE_ID_AUDIO_PCM)) {
                return BLT_FAILURE;
            }

            /* compute the match level */
            if (constructor->name != NULL) {
                /* we're being probed by name */
                if (ATX_StringsEqual(constructor->name, "VorbisDecoder")) {
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

            ATX_LOG_FINE_1("VorbisDecoderModule::Probe - Ok [%d]", *match); 
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(VorbisDecoderModule)
    ATX_GET_INTERFACE_ACCEPT_EX(VorbisDecoderModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(VorbisDecoderModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(VorbisDecoderModule, VorbisDecoder)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(VorbisDecoderModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    VorbisDecoderModule_Attach,
    VorbisDecoderModule_CreateInstance,
    VorbisDecoderModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define VorbisDecoderModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(VorbisDecoderModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(VorbisDecoderModule,
                                         "Vorbis Audio Decoder",
                                         "com.axiosys.decoder.vorbis",
                                         "1.1.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
