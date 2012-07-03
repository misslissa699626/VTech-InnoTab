/*****************************************************************
|
|   Stream Packetizer Module
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
#include "BltStreamPacketizer.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMediaPort.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltByteStreamUser.h"
#include "BltPacketProducer.h"
#include "BltPacketConsumer.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.general.stream-packetizer")

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);
} StreamPacketizerModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamUser);

    /* members */
    ATX_InputStream* stream;
    BLT_MediaType*   media_type;
    BLT_Boolean      eos;
} StreamPacketizerInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    BLT_MediaPacket* packet;
    BLT_Size         packet_size;
    BLT_Cardinal     packet_count;
    ATX_Int64        sample_count;
} StreamPacketizerOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    StreamPacketizerInput  input;
    StreamPacketizerOutput output;
} StreamPacketizer;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_STREAM_PACKETIZER_DEFAULT_PACKET_SIZE        4096
#define BLT_STREAM_PACKETIZER_DEFAULT_PACKET_SIZE_24BITS 6144 /* 24*256 */

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(StreamPacketizerModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(StreamPacketizer, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(StreamPacketizer, ATX_Referenceable)

/*----------------------------------------------------------------------
|   StreamPacketizerInput_SetStream
+---------------------------------------------------------------------*/
BLT_METHOD
StreamPacketizerInput_SetStream(BLT_InputStreamUser* _self,
                                ATX_InputStream*     stream,
                                const BLT_MediaType* media_type)
{
    StreamPacketizer* self = ATX_SELF_M(input, StreamPacketizer, BLT_InputStreamUser);

    /* if we had a stream, release it */
    ATX_RELEASE_OBJECT(self->input.stream);

    /* keep a reference to the stream */
    self->input.stream = stream;
    ATX_REFERENCE_OBJECT(stream);

    /* keep the media type */
    BLT_MediaType_Free(self->input.media_type);
    if (media_type) {
        BLT_MediaType_Clone(media_type, &self->input.media_type);

        /* update the packet size if we're in 24 bits per sample */
        if (media_type->id == BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
            const BLT_PcmMediaType* pcm_type = (const BLT_PcmMediaType*)media_type;
            if (((pcm_type->bits_per_sample+7)/8) == 3) {
                self->output.packet_size = BLT_STREAM_PACKETIZER_DEFAULT_PACKET_SIZE_24BITS;
            }
        }
    } else {
        BLT_MediaType_Clone(&BLT_MediaType_Unknown, &self->input.media_type);
    }
    
    /* reset the packet count */
    self->output.packet_count = 0;
    self->output.sample_count = 0;

    /* release anything we may have buffered */
    if (self->output.packet) {
        BLT_MediaPacket_Release(self->output.packet);
        self->output.packet = NULL;
    }
    
    /* reset the eos flag */
    self->input.eos = BLT_FALSE;

    /* update the stream info */
    {
        BLT_StreamInfo info;
        ATX_LargeSize  stream_size = 0;
        BLT_Result     result;

        result = ATX_InputStream_GetSize(stream, &stream_size);
        if (BLT_SUCCEEDED(result)) {
            if (ATX_BASE(self, BLT_BaseMediaNode).context) {
                info.mask = BLT_STREAM_INFO_MASK_SIZE;
                info.size = stream_size;
                BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &info);
            }
        }
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(StreamPacketizerInput)
    ATX_GET_INTERFACE_ACCEPT(StreamPacketizerInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(StreamPacketizerInput, BLT_InputStreamUser)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_InputStreamUser interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(StreamPacketizerInput, BLT_InputStreamUser)
    StreamPacketizerInput_SetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(StreamPacketizerInput,
                                         "input",
                                         STREAM_PULL,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(StreamPacketizerInput, BLT_MediaPort)
    StreamPacketizerInput_GetName,
    StreamPacketizerInput_GetProtocol,
    StreamPacketizerInput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    StreamPacketizerOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
StreamPacketizerOutput_GetPacket(BLT_PacketProducer* _self,
                                 BLT_MediaPacket**   packet)
{
    StreamPacketizer* self = ATX_SELF_M(output, StreamPacketizer, BLT_PacketProducer);
    BLT_Size          bytes_buffered;
    BLT_Size          bytes_read = 0;
    BLT_Result        result;

    /* default value */
    *packet = NULL;

    /* check for EOS */
    if (self->input.eos) {
        return BLT_ERROR_EOS;
    }

    if (self->output.packet == NULL) {
        /* get a packet from the core */
        result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
                                            self->output.packet_size,
                                            self->input.media_type,
                                            &self->output.packet);
        if (BLT_FAILED(result)) return result;
    }
    
    /* compute how many bytes we have already buffered */
    bytes_buffered = BLT_MediaPacket_GetPayloadSize(self->output.packet);
    
    /* read more data if necessary to fill the buffer */
    if (bytes_buffered < self->output.packet_size) {
        /* get the addr of the buffer */
        unsigned char* buffer = BLT_MediaPacket_GetPayloadBuffer(self->output.packet);

        /* read some data from the input stream */
        result = ATX_InputStream_Read(self->input.stream,
                                      buffer+bytes_buffered,
                                      self->output.packet_size-bytes_buffered,
                                      &bytes_read);
        if (BLT_FAILED(result)) {
            if (result == BLT_ERROR_EOS) {
                self->input.eos = BLT_TRUE;
                BLT_MediaPacket_SetFlags(self->output.packet, 
                                         BLT_MEDIA_PACKET_FLAG_END_OF_STREAM);
            } else {
                return result;
            }
        }

        /* update the size of the packet */
        bytes_buffered += bytes_read;
        BLT_MediaPacket_SetPayloadSize(self->output.packet, bytes_buffered);
    }
    
    /* if the buffer is not full, return now unless we're at the end of the stream */
    if (bytes_buffered != self->output.packet_size && !self->input.eos) {
        ATX_LOG_FINEST_2("StreamPacketizerOutput::GetPacket - buffer not full (%d of %d)",
                         bytes_buffered, self->output.packet_size);
        return BLT_ERROR_PORT_HAS_NO_DATA;
    }
    
    /* we're returning the output packet, so we do not keep a handle to it */
    *packet = self->output.packet;
    self->output.packet = NULL;
    
    /* set flags */     
    if (self->output.packet_count == 0) {
        /* this is the first packet */
        BLT_MediaPacket_SetFlags(*packet,
                                 BLT_MEDIA_PACKET_FLAG_START_OF_STREAM);
    }

    /* update the packet count */
    self->output.packet_count++;

    /* update the sample count and timestamp */
    if (self->input.media_type->id == BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
        BLT_PcmMediaType* pcm_type = (BLT_PcmMediaType*)self->input.media_type;
        if (pcm_type->channel_count   != 0 && 
            pcm_type->bits_per_sample != 0 &&
            pcm_type->sample_rate     != 0) {
            BLT_UInt32    sample_count;
            BLT_TimeStamp time_stamp;
    
            /* compute time stamp */
            time_stamp = BLT_TimeStamp_FromSamples(self->output.sample_count,
                                                   pcm_type->sample_rate);
            BLT_MediaPacket_SetTimeStamp(*packet, time_stamp);

            /* update sample count */
            sample_count = bytes_read/(pcm_type->channel_count*
                                       pcm_type->bits_per_sample/8);
            self->output.sample_count += sample_count;

            /* set the packet duration */
            BLT_MediaPacket_SetDuration(*packet, BLT_TimeStamp_FromSamples(sample_count, 
                                                                           pcm_type->sample_rate));
        }
    } 

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(StreamPacketizerOutput)
    ATX_GET_INTERFACE_ACCEPT(StreamPacketizerOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(StreamPacketizerOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(StreamPacketizerOutput,
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(StreamPacketizerOutput, BLT_MediaPort)
    StreamPacketizerOutput_GetName,
    StreamPacketizerOutput_GetProtocol,
    StreamPacketizerOutput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(StreamPacketizerOutput, BLT_PacketProducer)
    StreamPacketizerOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    StreamPacketizer_Create
+---------------------------------------------------------------------*/
static BLT_Result
StreamPacketizer_Create(BLT_Module*              module,
                        BLT_Core*                core, 
                        BLT_ModuleParametersType parameters_type,
                        BLT_CString              parameters, 
                        BLT_MediaNode**          object)
{
    StreamPacketizer* self;

    ATX_LOG_FINE("StreamPacketizer::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(StreamPacketizer));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* construct the object */
    self->input.stream       = NULL;
    self->input.eos          = BLT_FALSE;
    BLT_MediaType_Clone(&BLT_MediaType_None, &self->input.media_type);
    self->output.packet_size  = BLT_STREAM_PACKETIZER_DEFAULT_PACKET_SIZE;
    self->output.packet_count = 0;

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, StreamPacketizer, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, StreamPacketizer, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  StreamPacketizerInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  StreamPacketizerInput,  BLT_InputStreamUser);
    ATX_SET_INTERFACE(&self->output, StreamPacketizerOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, StreamPacketizerOutput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    StreamPacketizer_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
StreamPacketizer_Destroy(StreamPacketizer* self)
{
    ATX_LOG_FINE("StreamPacketizer::Destroy");

    /* the input stream should have been released when we were deactivated */
    ATX_ASSERT(self->input.stream == NULL);

    /* free the media type extensions */
    BLT_MediaType_Free(self->input.media_type);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    StreamPacketizer_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
StreamPacketizer_Deactivate(BLT_MediaNode* _self)
{
    StreamPacketizer* self = ATX_SELF_EX(StreamPacketizer, BLT_BaseMediaNode, BLT_MediaNode);
    
    ATX_LOG_FINE("StreamPacketizer::Deactivate");
    
    /* call the base class method */
    BLT_BaseMediaNode_Deactivate(_self);
    
    /* release the output packet if we still hold one */
    if (self->output.packet) {
        BLT_MediaPacket_Release(self->output.packet);
        self->output.packet = NULL;
    }

    /* release the input stream */
    ATX_RELEASE_OBJECT(self->input.stream);
       
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   StreamPacketizer_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
StreamPacketizer_GetPortByName(BLT_MediaNode*  _self,   
                               BLT_CString     name,
                               BLT_MediaPort** port)
{
    StreamPacketizer* self = ATX_SELF_EX(StreamPacketizer, BLT_BaseMediaNode, BLT_MediaNode);

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
|    StreamPacketizer_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
StreamPacketizer_Seek(BLT_MediaNode* _self,
                      BLT_SeekMode*  mode,
                      BLT_SeekPoint* point)
{
    StreamPacketizer* self = ATX_SELF_EX(StreamPacketizer, BLT_BaseMediaNode, BLT_MediaNode);
    BLT_Result        result;

    /* clear any end-of-stream condition */
    self->input.eos = BLT_FALSE;

    /* estimate the seek offset from the other stream parameters */
    result = BLT_Stream_EstimateSeekPoint(ATX_BASE(self, BLT_BaseMediaNode).context, *mode, point);
    if (BLT_FAILED(result)) return result;

    ATX_LOG_FINER_1("StreamPacketizer::Seek - seek offset = %d", (int)point->offset);

    /* seek into the input stream (ignore return value) */
    ATX_InputStream_Seek(self->input.stream, point->offset);

    /* update the current sample */
    if (point->mask & BLT_SEEK_POINT_MASK_SAMPLE) {
        self->output.sample_count = point->sample;
    }

    /* release anything we may have buffered */
    if (self->output.packet) {
        BLT_MediaPacket_Release(self->output.packet);
        self->output.packet = NULL;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(StreamPacketizer)
    ATX_GET_INTERFACE_ACCEPT_EX(StreamPacketizer, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(StreamPacketizer, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(StreamPacketizer, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    StreamPacketizer_GetPortByName,
    BLT_BaseMediaNode_Activate,
    StreamPacketizer_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    StreamPacketizer_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(StreamPacketizer, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   StreamPacketizerModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
StreamPacketizerModule_Probe(BLT_Module*              self, 
                             BLT_Core*                core,
                             BLT_ModuleParametersType parameters_type,
                             BLT_AnyConst             parameters,
                             BLT_Cardinal*            match)
{
    BLT_COMPILER_UNUSED(self);
    BLT_COMPILER_UNUSED(core);

    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

            /* the input protocol should be  STREAM_PULL and the */
            /* output protocol should be PACKET                  */
             if ((constructor->spec.input.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                  constructor->spec.input.protocol != BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL) ||
                 (constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                  constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_PACKET)) {
                return BLT_FAILURE;
            }

            /* media types must match */
            if (constructor->spec.input.media_type->id  != BLT_MEDIA_TYPE_ID_UNKNOWN &&
                constructor->spec.output.media_type->id != BLT_MEDIA_TYPE_ID_UNKNOWN &&
                constructor->spec.input.media_type->id  != constructor->spec.output.media_type->id) {
                return BLT_FAILURE;
            }

            /* compute the match level */
            if (constructor->name != NULL) {
                /* we're being probed by name */
                if (ATX_StringsEqual(constructor->name, "StreamPacketizer")) {
                    /* our name */
                    *match = BLT_MODULE_PROBE_MATCH_EXACT;
                } else {
                    /* not our name */
                    return BLT_FAILURE;
                }
            } else {
                /* we're probed by protocol/type specs only */
                *match = BLT_MODULE_PROBE_MATCH_DEFAULT;
            }

            ATX_LOG_FINE_1("StreamPacketizerModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(StreamPacketizerModule)
    ATX_GET_INTERFACE_ACCEPT_EX(StreamPacketizerModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(StreamPacketizerModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(StreamPacketizerModule, StreamPacketizer)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(StreamPacketizerModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    StreamPacketizerModule_CreateInstance,
    StreamPacketizerModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define StreamPacketizerModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(StreamPacketizerModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(StreamPacketizerModule,
                                         "Stream Packetizer",
                                         "com.axiosys.general.stream-packetizer",
                                         "1.0.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
