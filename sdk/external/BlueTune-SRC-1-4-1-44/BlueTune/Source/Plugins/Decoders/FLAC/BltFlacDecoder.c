/*****************************************************************
|
|   Flac Decoder Module
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
#include "BltFlacDecoder.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltPacketConsumer.h"
#include "BltByteStreamUser.h"
#include "BltStream.h"
#include "BltReplayGain.h"

#include "FLAC/stream_decoder.h"
/*#include "FLAC/seekable_stream_decoder.h"*/

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.decoders.flac")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_UInt32 flac_type_id;
} FlacDecoderModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamUser);

    /* members */
    BLT_Boolean                     eos;
    ATX_InputStream*                stream;
    BLT_LargeSize                   size;
    BLT_MediaTypeId                 media_type_id;
    FLAC__StreamDecoder*            decoder;
    FLAC__StreamMetadata_StreamInfo stream_info;
} FlacDecoderInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    BLT_PcmMediaType media_type;
    ATX_List*        packets;
    BLT_Cardinal     packet_count;
    BLT_Boolean      eos;
} FlacDecoderOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    FlacDecoderInput  input;
    FlacDecoderOutput output;
} FlacDecoder;

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(FlacDecoderModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(FlacDecoder, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(FlacDecoder, ATX_Referenceable)

/*----------------------------------------------------------------------
|   FlacDecoderInput_SetStream
+---------------------------------------------------------------------*/
BLT_METHOD
FlacDecoderInput_SetStream(BLT_InputStreamUser* _self, 
                           ATX_InputStream*     stream,
                           const BLT_MediaType* media_type)
{
    FlacDecoder* self = ATX_SELF_M(input, FlacDecoder, BLT_InputStreamUser);;

    /* check the stream's media type */
    if (media_type == NULL || 
        media_type->id != self->input.media_type_id) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* if we had a stream, release it */
    ATX_RELEASE_OBJECT(self->input.stream);

    /* keep a reference to the stream */
    self->input.stream = stream;
    ATX_REFERENCE_OBJECT(stream);

    /* reset counters and flags */
    self->input.size = 0;
    self->input.eos = BLT_FALSE;
    self->output.eos = BLT_FALSE;
    self->output.packet_count = 0;

    /* get stream size */
    ATX_InputStream_GetSize(stream, &self->input.size);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(FlacDecoderInput)
    ATX_GET_INTERFACE_ACCEPT(FlacDecoderInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(FlacDecoderInput, BLT_InputStreamUser)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_InputStreamUser interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(FlacDecoderInput, BLT_InputStreamUser)
    FlacDecoderInput_SetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(FlacDecoderInput,
                                         "input",
                                         STREAM_PULL,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(FlacDecoderInput, BLT_MediaPort)
    FlacDecoderInput_GetName,
    FlacDecoderInput_GetProtocol,
    FlacDecoderInput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    FlacDecoderOutput_Flush
+---------------------------------------------------------------------*/
BLT_METHOD
FlacDecoderOutput_Flush(FlacDecoder* self)
{
    ATX_ListItem* item;
    while ((item = ATX_List_GetFirstItem(self->output.packets))) {
        BLT_MediaPacket* packet = ATX_ListItem_GetData(item);
        if (packet) BLT_MediaPacket_Release(packet);
        ATX_List_RemoveItem(self->output.packets, item);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    FlacDecoderOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
FlacDecoderOutput_GetPacket(BLT_PacketProducer* _self,
                            BLT_MediaPacket**   packet)
{
    FlacDecoder* self = ATX_SELF_M(output, FlacDecoder, BLT_PacketProducer);
    FLAC__StreamDecoderState flac_state;
    FLAC__bool               flac_result = 0;
    BLT_Result               result;

    /* check for EOS */
    if (self->output.eos) {
        *packet = NULL;
        return BLT_ERROR_EOS;
    }

    /* decode until we have some data available */
    do {
        ATX_ListItem* item;
        item = ATX_List_GetFirstItem(self->output.packets);
        if (item != NULL) {
            *packet = ATX_ListItem_GetData(item);
            ATX_List_RemoveItem(self->output.packets, item);
            
            /* set flags */     
            if (self->output.packet_count == 0) {
                /* this is the first packet */
                BLT_MediaPacket_SetFlags(*packet,
                                        BLT_MEDIA_PACKET_FLAG_START_OF_STREAM);
            }

            /* update packet count */
            self->output.packet_count++;

            return BLT_SUCCESS;
        }

        /* no more data available, decode some more */
        flac_state = FLAC__stream_decoder_get_state(self->input.decoder);
        if (flac_state != FLAC__STREAM_DECODER_END_OF_STREAM) {
            flac_result = FLAC__stream_decoder_process_single(self->input.decoder);
        } else {
            break;
        }
    } while (flac_result == 1);
             
    /* return an empty packet with EOS flag */
    self->output.eos = BLT_TRUE;
    result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
                                        0,
                                        (BLT_MediaType*)&self->output.media_type,
                                        packet);
    if (BLT_FAILED(result)) return result;
    BLT_MediaPacket_SetFlags(*packet, 
                             BLT_MEDIA_PACKET_FLAG_END_OF_STREAM);

    /* flush the decoder, just in case... */
    FLAC__stream_decoder_flush(self->input.decoder);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(FlacDecoderOutput)
    ATX_GET_INTERFACE_ACCEPT(FlacDecoderOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(FlacDecoderOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(FlacDecoderOutput,
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(FlacDecoderOutput, BLT_MediaPort)
    FlacDecoderOutput_GetName,
    FlacDecoderOutput_GetProtocol,
    FlacDecoderOutput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(FlacDecoderOutput, BLT_PacketProducer)
    FlacDecoderOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   FlacDecoder_ReadCallback
+---------------------------------------------------------------------*/
static FLAC__StreamDecoderReadStatus 
FlacDecoder_ReadCallback(const FLAC__StreamDecoder* flac, 
                         FLAC__byte*                buffer, 
                         size_t*                    bytes, 
                         void*                      client_data)
{
    FlacDecoder*  self = (FlacDecoder*)client_data;
    BLT_Size      bytes_to_read = (BLT_Size)*bytes;
    BLT_Size      total_read    = 0;
    BLT_Result    result;

    /* unused parameters */
    BLT_COMPILER_UNUSED(flac);

    /* default value */
    *bytes = 0;
    
    /* check for EOS */
    if (self->input.eos) {
        return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }
    
    /* read from the input stream until we've filled the buffer */
    /* or reached the end.                                      */
    while (bytes_to_read) {
        BLT_Size bytes_read = 0;
        ATX_LOG_FINEST_1("requesting %d bytes", bytes_to_read);
        result = ATX_InputStream_Read(self->input.stream,
                                      buffer,
                                      bytes_to_read,
                                      &bytes_read);
        if (BLT_FAILED(result)) {
            if (result == BLT_ERROR_EOS) {
                self->input.eos = BLT_TRUE;
                if (total_read == 0) {
                    /* nothing was read at all */
                    ATX_LOG_FINEST("nothing was read, EOF");
                    return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
                } else {
                    break;
                }
            }
            return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
        }
        
        if (bytes_read <= bytes_to_read) {
            bytes_to_read -= bytes_read;
            total_read    += bytes_read;
            buffer        += bytes_read;
        } else {
            /* something's wrong */
            return BLT_ERROR_INTERNAL;
        }
    }
    
    ATX_LOG_FINER_1("total read = %d", total_read);
    *bytes = total_read;
    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

/*----------------------------------------------------------------------
|   FlacDecoder_SeekCallback
+---------------------------------------------------------------------*/
static FLAC__StreamDecoderSeekStatus 
FlacDecoder_SeekCallback(const FLAC__StreamDecoder *decoder, 
                         FLAC__uint64               offset, 
                         void*                      client_data)
{
    FlacDecoder* self = (FlacDecoder*)client_data;
    BLT_Result   result;

    /* unused parameters */
    BLT_COMPILER_UNUSED(decoder);

    /* clear the EOS flag */
    self->input.eos  = BLT_FALSE;
    self->output.eos = BLT_FALSE;

    /* seek */
    ATX_LOG_FINER_1("FlacDecoder::SeekCallback - offset = %ld", offset);
    result = ATX_InputStream_Seek(self->input.stream, (ATX_Position)offset);
    if (BLT_FAILED(result)) {
        return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
    }

    return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

/*----------------------------------------------------------------------
|   FlacDecoder_TellCallback
+---------------------------------------------------------------------*/
static FLAC__StreamDecoderTellStatus 
FlacDecoder_TellCallback(const FLAC__StreamDecoder* flac, 
                         FLAC__uint64*              offset, 
                         void*                      client_data)
{
    FlacDecoder* self = (FlacDecoder*)client_data;
    ATX_Position stream_offset;
    BLT_Result   result;

    /* unused parameters */
    BLT_COMPILER_UNUSED(flac);

    /* return the stream position */
    result = ATX_InputStream_Tell(self->input.stream, &stream_offset);
    if (BLT_FAILED(result)) {
        *offset = 0;
        return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
    }

    /*BLT_Debug("FlacDecoder::TellCallback - offset = %ld\n", *offset);*/
    *offset = stream_offset;

    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

/*----------------------------------------------------------------------
|   FlacDecoder_LengthCallback
+---------------------------------------------------------------------*/
static FLAC__StreamDecoderLengthStatus 
FlacDecoder_LengthCallback(const FLAC__StreamDecoder* decoder, 
                           FLAC__uint64*              stream_length, 
                           void*                      client_data)
{
    FlacDecoder*  self = (FlacDecoder*)client_data;
    BLT_LargeSize size;
    BLT_Result    result;

    /* unused parameters */
    BLT_COMPILER_UNUSED(decoder);

    /* get the stream size */
    result = ATX_InputStream_GetSize(self->input.stream, &size);
    if (BLT_FAILED(result)) {
        *stream_length = 0;
    } else {
        *stream_length = size;
    }

    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

/*----------------------------------------------------------------------
|   FlacDecoder_EofCallback
+---------------------------------------------------------------------*/
static FLAC__bool 
FlacDecoder_EofCallback(const FLAC__StreamDecoder* decoder, 
                        void*                      client_data)
{
    FlacDecoder* self = (FlacDecoder*)client_data;
    
    /* unused parameters */
    BLT_COMPILER_UNUSED(decoder);

    return self->input.eos == BLT_TRUE ? 1:0;
}

/*----------------------------------------------------------------------
|   FlacDecoder_WriteCallback
+---------------------------------------------------------------------*/
static FLAC__StreamDecoderWriteStatus 
FlacDecoder_WriteCallback(const FLAC__StreamDecoder* decoder, 
                          const FLAC__Frame*         frame, 
                          const FLAC__int32* const   buffer[], 
                          void*                      client_data)
{
    FlacDecoder*     self = (FlacDecoder*)client_data;
    BLT_MediaPacket* packet;
    BLT_Size         packet_size;
    BLT_Result       result;

    /* unused parameters */
    BLT_COMPILER_UNUSED(decoder);

    /* check format */
    if (frame->header.channels == 0 ||
        frame->header.channels > 2  ||
        frame->header.channels < 1) {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    /* support 16, 24 and 32 bps */
    if (frame->header.bits_per_sample != 16 &&
        frame->header.bits_per_sample != 24 &&
        frame->header.bits_per_sample != 32) {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    /* compute packet size */
    packet_size = frame->header.blocksize * frame->header.channels * frame->header.bits_per_sample/8;
    
    /* set the packet media type */
    self->output.media_type.sample_rate     = frame->header.sample_rate;
    self->output.media_type.channel_count   = frame->header.channels;
    self->output.media_type.channel_mask    = 0;
    self->output.media_type.bits_per_sample = frame->header.bits_per_sample;
    self->output.media_type.sample_format   = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE;

    /* get a packet from the core */
    result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
                                        packet_size,
                                        (BLT_MediaType*)&self->output.media_type,
                                        &packet);
    if (BLT_FAILED(result)) return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

    /* convert sample buffer */
    if (frame->header.channels == 2) {
        unsigned int sample;
        switch (frame->header.bits_per_sample) {
            case 16:
                {
                    short* dst = (short*)BLT_MediaPacket_GetPayloadBuffer(packet);
                    for (sample = 0; sample < frame->header.blocksize; sample++) {
                        *dst++ = buffer[0][sample];
                        *dst++ = buffer[1][sample];
                    }
                }
                break;

            case 24:
                {
                    unsigned char* dst = (unsigned char*)BLT_MediaPacket_GetPayloadBuffer(packet);
                    for (sample = 0; sample < frame->header.blocksize; sample++) {
                        unsigned char* src_0 = (unsigned char*)&(buffer[0][sample]);
                        unsigned char* src_1 = (unsigned char*)&(buffer[1][sample]);
                        *dst++ = src_0[0];
                        *dst++ = src_0[1];
                        *dst++ = src_0[2];
                        *dst++ = src_1[0];
                        *dst++ = src_1[1];
                        *dst++ = src_1[2];
                    }
                }
                break;

            case 32:
                {
                    FLAC__int32* dst = (FLAC__int32*)BLT_MediaPacket_GetPayloadBuffer(packet);
                    for (sample = 0; sample < frame->header.blocksize; sample++) {
                        *dst++ = buffer[0][sample];
                        *dst++ = buffer[1][sample];
                    }
                }
                break;
        }
    } else {
        ATX_CopyMemory(BLT_MediaPacket_GetPayloadBuffer(packet), 
                       buffer[0], packet_size);
    }

    /* update the size of the packet */
    BLT_MediaPacket_SetPayloadSize(packet, packet_size);
    
    /* add the packet to our output queue */
    ATX_List_AddData(self->output.packets, packet);

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

/*----------------------------------------------------------------------
|   FlacDecoder_HandleStreamInfo
+---------------------------------------------------------------------*/
static void
FlacDecoder_HandleStreamInfo(
    FlacDecoder*                           self, 
    const FLAC__StreamMetadata_StreamInfo* flac_info)
{
    /* update the stream info */
    self->input.stream_info = *flac_info;
    if (ATX_BASE(self, BLT_BaseMediaNode).context) {
        BLT_StreamInfo info;
        FLAC__uint64   duration;
        FLAC__uint64   bitrate;

        /* start with no info */
        info.mask = 0;

        /* sample rate */
        info.sample_rate = flac_info->sample_rate;
        info.mask |= BLT_STREAM_INFO_MASK_SAMPLE_RATE;

        /* channel count */
        info.channel_count = flac_info->channels;
        info.mask |= BLT_STREAM_INFO_MASK_CHANNEL_COUNT;

        /* compute duration from samples and sample_rate */
        if (flac_info->sample_rate) {
            duration = (flac_info->total_samples*1000)/flac_info->sample_rate;
            info.mask |= BLT_STREAM_INFO_MASK_DURATION;
        } else {
            duration = 0;
        }
        info.duration = (BLT_Cardinal)duration;

        /* compute bitrate from input size and duration */
        if (duration) {
            bitrate = 8*1000*(FLAC__uint64)self->input.size/duration;
            info.mask |= BLT_STREAM_INFO_MASK_NOMINAL_BITRATE;
            info.mask |= BLT_STREAM_INFO_MASK_AVERAGE_BITRATE;
            info.mask |= BLT_STREAM_INFO_MASK_INSTANT_BITRATE;
        } else {
            bitrate = 0;
        }
        info.nominal_bitrate = (BLT_Cardinal)bitrate;
        info.average_bitrate = (BLT_Cardinal)bitrate;
        info.instant_bitrate = (BLT_Cardinal)bitrate;

        /* data type */
        info.data_type = "FLAC";
        info.mask |= BLT_STREAM_INFO_MASK_DATA_TYPE;

        /* send update */
        BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &info);
    }
}

/*----------------------------------------------------------------------
|   FlacDecoder_HandleVorbisComment
+---------------------------------------------------------------------*/
static void
FlacDecoder_HandleVorbisComment(
    FlacDecoder*                              self,
    const FLAC__StreamMetadata_VorbisComment* comment)
{
    unsigned int i;
    ATX_String   string = ATX_EMPTY_STRING;
    ATX_String   key    = ATX_EMPTY_STRING;
    ATX_String   value  = ATX_EMPTY_STRING;
    float        track_gain = 0.0f;
    float        album_gain = 0.0f;
    BLT_ReplayGainSetMode track_gain_mode = BLT_REPLAY_GAIN_SET_MODE_IGNORE;
    BLT_ReplayGainSetMode album_gain_mode = BLT_REPLAY_GAIN_SET_MODE_IGNORE;

    ATX_String_AssignN(&string,
                       (const char*)comment->vendor_string.entry,
                       comment->vendor_string.length);
    ATX_LOG_FINER_1("VENDOR = %s", ATX_CSTR(string));
    for (i=0; i<comment->num_comments; i++) {
        int sep;
        ATX_String_AssignN(&string, 
                           (const char*)comment->comments[i].entry,
                           comment->comments[i].length);
        sep = ATX_String_FindChar(&string, '=');
        if (sep == ATX_STRING_SEARCH_FAILED) continue;
        ATX_String_AssignN(&key, ATX_CSTR(string), sep);
        ATX_String_Assign(&value, ATX_CSTR(string)+sep+1);

        ATX_LOG_FINER_3("  COMMENT %d : %s = %s", i, ATX_CSTR(key), ATX_CSTR(value));
        ATX_String_ToUppercase(&key);
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

    
/*----------------------------------------------------------------------
|   FlacDecoder_MetaDataCallback
+---------------------------------------------------------------------*/
static void 
FlacDecoder_MetaDataCallback(const FLAC__StreamDecoder*  decoder, 
                             const FLAC__StreamMetadata* metadata, 
                             void*                       client_data)
{
    FlacDecoder* self = (FlacDecoder*)client_data;
    
    /* unused parameters */
    BLT_COMPILER_UNUSED(decoder);

    /* get metadata block */
    switch (metadata->type) {
      case FLAC__METADATA_TYPE_STREAMINFO:
        FlacDecoder_HandleStreamInfo(self, &metadata->data.stream_info);
        break;
        
      case FLAC__METADATA_TYPE_VORBIS_COMMENT:
        FlacDecoder_HandleVorbisComment(self, &metadata->data.vorbis_comment);
        break;

      default:
        break;
    }
}

/*----------------------------------------------------------------------
|   FlacDecoder_ErrorCallback
+---------------------------------------------------------------------*/
static void 
FlacDecoder_ErrorCallback(const FLAC__StreamDecoder*     decoder, 
                          FLAC__StreamDecoderErrorStatus status, 
                          void*                          client_data)
{
    /* IGNORE */
    BLT_COMPILER_UNUSED(decoder);
    BLT_COMPILER_UNUSED(status);
    BLT_COMPILER_UNUSED(client_data);

    /*BLT_Debug("FlacDecoder::ErrorCallback (%d)\n", status);*/
}

/*----------------------------------------------------------------------
|    FlacDecoder_SetupPorts
+---------------------------------------------------------------------*/
static BLT_Result
FlacDecoder_SetupPorts(FlacDecoder* self, BLT_MediaTypeId flac_type_id)
{
    ATX_Result result;

    /* setup the input port */
    self->input.eos = BLT_FALSE;
    self->input.stream = NULL;
    self->input.media_type_id = flac_type_id;

    /* setup the output port */
    self->output.eos                    = BLT_FALSE;
    self->output.packet_count           = 0;
    BLT_PcmMediaType_Init(&self->output.media_type);

    /* create a list of output packets */
    result = ATX_List_Create(&self->output.packets);
    if (ATX_FAILED(result)) return result;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    FlacDecoder_Create
+---------------------------------------------------------------------*/
static BLT_Result
FlacDecoder_Create(BLT_Module*              module,
                   BLT_Core*                core, 
                   BLT_ModuleParametersType parameters_type,
                   BLT_CString              parameters, 
                   BLT_MediaNode**          object)
{
    FlacDecoder*                  self;
    BLT_Result                    result;
    FLAC__StreamDecoderInitStatus init_status;;

    ATX_LOG_FINE("FlacDecoder::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    *object = NULL;
    self = ATX_AllocateZeroMemory(sizeof(FlacDecoder));
    if (self == NULL) {
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* create FLAC self */
    self->input.decoder = FLAC__stream_decoder_new();

    /* setup the flac decoder */
    init_status = FLAC__stream_decoder_init_stream(
        self->input.decoder,
        FlacDecoder_ReadCallback,
        FlacDecoder_SeekCallback,
        FlacDecoder_TellCallback,
        FlacDecoder_LengthCallback,
        FlacDecoder_EofCallback,
        FlacDecoder_WriteCallback,
        FlacDecoder_MetaDataCallback,
        FlacDecoder_ErrorCallback,
        self);
    if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        ATX_LOG_WARNING_1("FlacDecoder_Create - FLAC__stream_decoder_init_stream failed (%d)", (int)init_status);

        FLAC__stream_decoder_delete(self->input.decoder);
        ATX_FreeMemory((void*)self);
        return BLT_FAILURE;
    }
    FLAC__stream_decoder_set_metadata_respond(self->input.decoder,
        FLAC__METADATA_TYPE_VORBIS_COMMENT);

    /* setup the input and output ports */
    result = FlacDecoder_SetupPorts(self, 
                                    ATX_SELF_EX_O(module, FlacDecoderModule, BLT_BaseModule, BLT_Module)->flac_type_id);
    if (BLT_FAILED(result)) {
        ATX_FreeMemory(self);
        *object = NULL;
        return result;
    }

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, FlacDecoder, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, FlacDecoder, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  FlacDecoderInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  FlacDecoderInput,  BLT_InputStreamUser);
    ATX_SET_INTERFACE(&self->output, FlacDecoderOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, FlacDecoderOutput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    FlacDecoder_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
FlacDecoder_Destroy(FlacDecoder* self)
{
    ATX_ListItem* item;
    
    ATX_LOG_FINE("FlacDecoder::Destroy");

    /* release the input stream */
    ATX_RELEASE_OBJECT(self->input.stream);

    /* release any output packet we may hold */
    item = ATX_List_GetFirstItem(self->output.packets);
    while (item) {
        BLT_MediaPacket* packet = ATX_ListItem_GetData(item);
        if (packet) {
            BLT_MediaPacket_Release(packet);
        }
        item = ATX_ListItem_GetNext(item);
    }
    ATX_List_Destroy(self->output.packets);
    
    /* destroy the FLAC decoder */
    if (self->input.decoder) {
        FLAC__stream_decoder_delete(self->input.decoder);
    }
    
    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}
           
/*----------------------------------------------------------------------
|    FlacDecoder_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
FlacDecoder_Deactivate(BLT_MediaNode* _self)
{
    FlacDecoder* self = ATX_SELF_EX(FlacDecoder, BLT_BaseMediaNode, BLT_MediaNode);

    ATX_LOG_FINER("FlacDecoder::Deactivate");

    /* call the base class method */
    BLT_BaseMediaNode_Deactivate(_self);

    /* release the input stream */
    ATX_RELEASE_OBJECT(self->input.stream);

    return BLT_SUCCESS;
}
                    
/*----------------------------------------------------------------------
|   FlacDecoder_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
FlacDecoder_GetPortByName(BLT_MediaNode*  _self,
                          BLT_CString     name,
                          BLT_MediaPort** port)
{
    FlacDecoder* self = ATX_SELF_EX(FlacDecoder, BLT_BaseMediaNode, BLT_MediaNode);

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
|    FlacDecoder_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
FlacDecoder_Seek(BLT_MediaNode* _self,
                 BLT_SeekMode*  mode,
                 BLT_SeekPoint* point)
{
    FlacDecoder* self = ATX_SELF_EX(FlacDecoder, BLT_BaseMediaNode, BLT_MediaNode);
    FLAC__bool   result;

    /* flush pending packets */
    FlacDecoderOutput_Flush(self);

    /* estimate the seek point in time_stamp mode */
    if (ATX_BASE(self, BLT_BaseMediaNode).context == NULL) return BLT_FAILURE;
    BLT_Stream_EstimateSeekPoint(ATX_BASE(self, BLT_BaseMediaNode).context, *mode, point);
    if (!(point->mask & BLT_SEEK_POINT_MASK_SAMPLE)) {
        return BLT_FAILURE;
    }

    /* seek to the target sample */
    ATX_LOG_FINE_1("FlacDecoder::Seek - sample = %ld", (long)point->sample);
    FLAC__stream_decoder_flush(self->input.decoder);
    result = FLAC__stream_decoder_seek_absolute(self->input.decoder, point->sample);

    /* set the mode so that the nodes down the chain know the seek has */
    /* already been done on the stream                                 */
    *mode = BLT_SEEK_MODE_IGNORE;

    return result == result?BLT_SUCCESS:BLT_FAILURE;
}

/*----------------------------------------------------------------------
|    GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(FlacDecoder)
ATX_GET_INTERFACE_ACCEPT_EX(FlacDecoder, BLT_BaseMediaNode, BLT_MediaNode)
ATX_GET_INTERFACE_ACCEPT_EX(FlacDecoder, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(FlacDecoder, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    FlacDecoder_GetPortByName,
    BLT_BaseMediaNode_Activate,
    FlacDecoder_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    FlacDecoder_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|    ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(FlacDecoder, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|    FlacDecoderModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
FlacDecoderModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    FlacDecoderModule* self = ATX_SELF_EX(FlacDecoderModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*      registry;
    BLT_Result         result;

    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the ".flac" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".flac",
                                            "audio/x-flac");
    if (BLT_FAILED(result)) return result;

    /* register the "audio/x-flac" type */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/x-flac",
        &self->flac_type_id);
    if (BLT_FAILED(result)) return result;
    
    ATX_LOG_FINE_1("FlacDecoderModule::Attach (audio/x-flac type = %d)", self->flac_type_id);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   FlacDecoderModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
FlacDecoderModule_Probe(BLT_Module*              _self,  
                        BLT_Core*                core,
                        BLT_ModuleParametersType parameters_type,
                        BLT_AnyConst             parameters,
                        BLT_Cardinal*            match)
{
    FlacDecoderModule* self = ATX_SELF_EX(FlacDecoderModule, BLT_BaseModule, BLT_Module);
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

            /* the input type should be audio/x-flac */
            if (constructor->spec.input.media_type->id != 
                self->flac_type_id) {
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
                if (ATX_StringsEqual(constructor->name, "FlacDecoder")) {
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
            
            ATX_LOG_FINE_1("FlacDecoderModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(FlacDecoderModule)
ATX_GET_INTERFACE_ACCEPT_EX(FlacDecoderModule, BLT_BaseModule, BLT_Module)
ATX_GET_INTERFACE_ACCEPT_EX(FlacDecoderModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(FlacDecoderModule, FlacDecoder)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(FlacDecoderModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    FlacDecoderModule_Attach,
    FlacDecoderModule_CreateInstance,
    FlacDecoderModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define FlacDecoderModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(FlacDecoderModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(FlacDecoderModule,
                                         "FLAC Audio Decoder",
                                         "com.axiosys.decoder.flac",
                                         "1.1.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
