/*****************************************************************
|
|   BlueTune - Intel IPP Wrapper Decoder Module
|
|   (c) 2008-2009 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltCore.h"
#include "BltIppDecoder.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltMediaPacket.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltPacketConsumer.h"
#include "BltStream.h"
#include "BltCommonMediaTypes.h"
#include "BltPixels.h"

// IPP includes
#include "ippcore.h"
#define VM_MALLOC_GLOBAL
#include "umc_malloc.h"
#include "umc_video_decoder.h"
#include "umc_video_data.h"
#include "umc_video_processing.h"
#include "umc_data_pointers_copy.h"
#include "umc_h264_dec.h"
#include "umc_h264_timing.h"

using namespace UMC;

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.decoders.ipp")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_IPP_DECODER_FORMAT_TAG_AVC1 0x61766331    /* 'avc1' */

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_UInt32 mp4_video_es_type_id;
    BLT_UInt32 iso_base_video_es_type_id;
} IppDecoderModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketConsumer);

    /* members */
    MediaData*  buffer;
    BLT_Boolean eos;
} IppDecoderInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    BLT_Boolean           eos;
    BLT_RawVideoMediaType media_type;
    VideoData*            buffer;       
} IppDecoderOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    IppDecoderModule* module;
    IppDecoderInput   input;
    IppDecoderOutput  output;
    H264VideoDecoder* decoder;
    struct {
        vm_tick decode_time;
        vm_tick alloc_time;
        vm_tick convert_time;
    } stats;
} IppDecoder;

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
    
/*----------------------------------------------------------------------
|   IppDecoderInput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
IppDecoderInput_PutPacket(BLT_PacketConsumer* _self,
                          BLT_MediaPacket*    packet)
{
    IppDecoder* self   = ATX_SELF_M(input, IppDecoder, BLT_PacketConsumer);
    BLT_Result  result = BLT_SUCCESS;

    if (self->decoder == NULL) {
        ATX_LOG_FINE("instantiating new H.264 decoder");

        // check the media type and find the right codec
        const BLT_Mp4VideoMediaType* media_type;
        const unsigned char*         decoder_config = NULL;
        unsigned int                 decoder_config_size = 0;
        
        // check the packet type
        BLT_MediaPacket_GetMediaType(packet, (const BLT_MediaType**)&media_type);
        if ((media_type->base.base.id != self->module->iso_base_video_es_type_id &&
             media_type->base.base.id != self->module->mp4_video_es_type_id) ||
            media_type->base.stream_type != BLT_MP4_STREAM_TYPE_VIDEO) {
            ATX_LOG_FINE("invalid media type");
            return BLT_ERROR_INVALID_MEDIA_TYPE;
        }
        if (media_type->base.format_or_object_type_id == BLT_IPP_DECODER_FORMAT_TAG_AVC1) {
            ATX_LOG_FINE("content type is AVC");
            decoder_config      = media_type->decoder_info;
            decoder_config_size = media_type->decoder_info_length;
        } else {
            ATX_LOG_FINE("unsupported codec");
            return BLT_ERROR_UNSUPPORTED_CODEC;
        }
        
        // codec init data
        MediaData codec_init_data(decoder_config_size);
        codec_init_data.SetDataSize(decoder_config_size);
        ATX_CopyMemory(codec_init_data.GetDataPointer(), decoder_config, decoder_config_size);
        
        // setup video processing params
        DataPointersCopy*       video_proc = new DataPointersCopy();
        //VideoProcessing       video_proc;
        //VideoProcessingParams video_proc_params;
        //video_proc_params.m_DeinterlacingMethod = NO_DEINTERLACING;
        //video_proc_params.InterpolationMethod = 0;
        //video_proc.SetParams(&video_proc_params);

        // setup video decoder params
        VideoDecoderParams dec_params;
        dec_params.pPostProcessing     = video_proc;
        dec_params.info.stream_type    = H264_VIDEO;
        dec_params.info.stream_subtype = AVC1_VIDEO;
        dec_params.numThreads          = 0; // default
        dec_params.lFlags              = UMC::FLAG_VDEC_REORDER;
        dec_params.m_pData             = &codec_init_data;
        
        // create and init the decoder
        self->decoder = new H264VideoDecoder();
        UMC::Status status = self->decoder->Init(&dec_params);
        if (status != UMC_OK) {
            ATX_LOG_WARNING_1("H264 decoder Init() failed (%d)", status);
            return BLT_FAILURE;
        }

        // get decoder params
        H264VideoDecoderParams h264_params;
        status = self->decoder->GetInfo(&h264_params);
        if (status != UMC_OK) {
            ATX_LOG_WARNING_1("H264 decoder GetInfo() failed (%d)", status);
            return BLT_FAILURE;
        }
        
        // allocate the input buffer
        delete self->input.buffer;
        self->input.buffer = new MediaData();
        
        // allocate the output buffer
        delete self->output.buffer;
        self->output.buffer = new VideoData();
        self->output.buffer->SetAlignment(16);
        self->output.buffer->Init(h264_params.info.clip_info.width, 
                                  h264_params.info.clip_info.height, 
                                  UMC::YV12, 
                                  8);
        self->output.buffer->Alloc();        
    }

    // check to see if this is the end of a stream
    if (BLT_MediaPacket_GetFlags(packet) & BLT_MEDIA_PACKET_FLAG_END_OF_STREAM) {
        self->input.eos = BLT_TRUE;
    }
    
    // copy the data to the input buffer
    unsigned int payload_size = BLT_MediaPacket_GetPayloadSize(packet);
    if (self->input.buffer->SetDataSize(payload_size) != UMC_OK) {
        // the buffer was too small, realloc
        self->input.buffer->Alloc(payload_size);
        self->input.buffer->SetDataSize(payload_size);
    }
    ATX_CopyMemory(self->input.buffer->GetDataPointer(),
                   BLT_MediaPacket_GetPayloadBuffer(packet),
                   BLT_MediaPacket_GetPayloadSize(packet));
    
    // set the buffer's timestamp
    self->input.buffer->SetTime(BLT_TimeStamp_ToSeconds(BLT_MediaPacket_GetTimeStamp(packet)));
    
    return result;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(IppDecoderInput)
    ATX_GET_INTERFACE_ACCEPT(IppDecoderInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(IppDecoderInput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(IppDecoderInput, BLT_PacketConsumer)
    IppDecoderInput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(IppDecoderInput, 
                                         "input",
                                         PACKET,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(IppDecoderInput, BLT_MediaPort)
    IppDecoderInput_GetName,
    IppDecoderInput_GetProtocol,
    IppDecoderInput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   IppDecoderOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
IppDecoderOutput_GetPacket(BLT_PacketProducer* _self,
                           BLT_MediaPacket**   packet)
{
    IppDecoder* self = ATX_SELF_M(output, IppDecoder, BLT_PacketProducer);
    
    // default return value
    *packet = NULL;

    // check that we have a decoder
    if (self->decoder == NULL) return BLT_ERROR_PORT_HAS_NO_DATA;

    // try to decode a frame
    UMC::Status status;
    vm_tick start_time = vm_time_get_tick();
    status = self->decoder->GetFrame(self->input.eos?NULL:self->input.buffer, 
                                     self->output.buffer);
    vm_tick after_decode_time = vm_time_get_tick();
    vm_tick decode_time = after_decode_time-start_time;
    self->stats.decode_time += decode_time;
    ATX_LOG_FINER_3("status = %d, decode: frame = %.4fms, total = %.4f", status, 
                    1000.0f*(float)decode_time/(float)vm_time_get_frequency(),
                    1000.0f*(float)self->stats.decode_time/(float)vm_time_get_frequency());
    if (status == UMC_ERR_NOT_ENOUGH_DATA) {
        return BLT_ERROR_PORT_HAS_NO_DATA;
    } else if (status != UMC_OK) {
        ATX_LOG_WARNING_1("GetFrame failed (%d)", status);
    } 
    
    // check the format
    if (self->output.buffer->GetColorFormat() != UMC::YUV420) {
        ATX_LOG_FINE("color format is not YUV420");
        return BLT_ERROR_UNSUPPORTED_FORMAT;
    }
    if (self->output.buffer->GetNumPlanes() != 3) {
        ATX_LOG_FINE("number of planes is not 3");
        return BLT_ERROR_UNSUPPORTED_FORMAT;
    }
    for (unsigned int i=0; i<3; i++) {
        VideoData::PlaneInfo plane_info;
        self->output.buffer->GetPlaneInfo(&plane_info, i);
        if (plane_info.m_iSamples != 1) {
            ATX_LOG_FINE("plane sample size is not 1");
            return BLT_ERROR_UNSUPPORTED_FORMAT;
        }
    }
    
    // create a media packet for the frame
    unsigned int   plane_size[3];
    unsigned int   padding_size[3] = {0,0,0};
    unsigned int   picture_size = 0;
    unsigned int   picture_width  = self->output.buffer->GetWidth();
    unsigned int   picture_height = self->output.buffer->GetHeight();
    unsigned int   i;
    
    ATX_LOG_FINEST_2("decoded frame width=%d, height=%d", picture_width, picture_height);
    self->output.media_type.width  = picture_width;
    self->output.media_type.height = picture_height;
    self->output.media_type.format = BLT_PIXEL_FORMAT_YV12;
    self->output.media_type.flags  = 0;

    for (i=0; i<3; i++) {
        VideoData::PlaneInfo plane_info;
        self->output.buffer->GetPlaneInfo(&plane_info, i);
        
        plane_size[i] = (unsigned int)plane_info.m_nPitch*plane_info.m_ippSize.height;
        if (plane_size[i]%16) {
            padding_size[i] = 16-(plane_size[i]%16);
        }
        self->output.media_type.planes[i].offset = picture_size;
        self->output.media_type.planes[i].bytes_per_line = (BLT_UInt16)plane_info.m_nPitch;
        picture_size += plane_size[i]+padding_size[i];
    }
    
    BLT_Result result;
    result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core, 
                                        picture_size, 
                                        &self->output.media_type.base, 
                                        packet);
    if (BLT_FAILED(result)) {
        ATX_LOG_WARNING_1("BLT_Core_CreateMediaPacket returned %d", result);
        return result;
    }
    vm_tick after_alloc_time = vm_time_get_tick();
    vm_tick alloc_time = after_alloc_time-after_decode_time;
    self->stats.alloc_time += alloc_time;
    ATX_LOG_FINER_2("alloc: frame = %.4fms, total = %.4f", 
                    1000.0f*(float)alloc_time/(float)vm_time_get_frequency(),
                    1000.0f*(float)self->stats.alloc_time/(float)vm_time_get_frequency());

    // copy pixels
    BLT_MediaPacket_SetPayloadSize(*packet, picture_size);
    unsigned char* picture_buffer = (unsigned char*)BLT_MediaPacket_GetPayloadBuffer(*packet);
    for (i=0; i<3; i++) {
        VideoData::PlaneInfo plane_info;
        self->output.buffer->GetPlaneInfo(&plane_info, i);
        ATX_CopyMemory(picture_buffer+self->output.media_type.planes[i].offset, 
                       plane_info.m_pPlane, 
                       plane_size[i]);
    }

    // set the timestamp
    BLT_MediaPacket_SetTimeStamp(*packet, BLT_TimeStamp_FromSeconds(self->output.buffer->GetTime()));
    
    // update timing stats
    vm_tick after_convert_time = vm_time_get_tick();
    vm_tick convert_time = after_convert_time-after_alloc_time;
    self->stats.convert_time += convert_time;
    ATX_LOG_FINER_2("convert: frame = %.4fms, total = %.4f", 
                    1000.0f*(float)convert_time/(float)vm_time_get_frequency(),
                    1000.0f*(float)self->stats.convert_time/(float)vm_time_get_frequency());

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(IppDecoderOutput)
    ATX_GET_INTERFACE_ACCEPT(IppDecoderOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(IppDecoderOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(IppDecoderOutput, 
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(IppDecoderOutput, BLT_MediaPort)
    IppDecoderOutput_GetName,
    IppDecoderOutput_GetProtocol,
    IppDecoderOutput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(IppDecoderOutput, BLT_PacketProducer)
    IppDecoderOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   IppDecoder_SetupPorts
+---------------------------------------------------------------------*/
static BLT_Result
IppDecoder_SetupPorts(IppDecoder* self)
{
    /*ATX_Result result;*/

    /* init the input port */
    self->input.eos = BLT_FALSE;
    
    /* setup the output port */
    self->output.eos = BLT_FALSE;
    BLT_RawVideoMediaType_Init(&self->output.media_type);
    self->output.media_type.format = BLT_PIXEL_FORMAT_YV12;
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    IppDecoder_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
IppDecoder_Destroy(IppDecoder* self)
{ 

    ATX_LOG_FINE("enter");

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));
    
    /* free resources */
    delete self->input.buffer;
    delete self->output.buffer;

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}
                    
/*----------------------------------------------------------------------
|   IppDecoder_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
IppDecoder_GetPortByName(BLT_MediaNode*  _self,
                         BLT_CString     name,
                         BLT_MediaPort** port)
{
    IppDecoder* self = ATX_SELF_EX(IppDecoder, BLT_BaseMediaNode, BLT_MediaNode);

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
|    IppDecoder_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
IppDecoder_Seek(BLT_MediaNode* _self,
                BLT_SeekMode*  mode,
                BLT_SeekPoint* point)
{
    IppDecoder* self = ATX_SELF_EX(IppDecoder, BLT_BaseMediaNode, BLT_MediaNode);

    BLT_COMPILER_UNUSED(mode);
    BLT_COMPILER_UNUSED(point);

    /* clear the eos flags */
    self->input.eos   = BLT_FALSE;
    self->output.eos  = BLT_FALSE;

    /* flush anything that may be pending */
    /*if (self->output.picture) {
        BLT_MediaPacket_Release(self->output.picture);
        self->output.picture = NULL;
    }*/

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(IppDecoder)
    ATX_GET_INTERFACE_ACCEPT_EX(IppDecoder, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(IppDecoder, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(IppDecoder, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    IppDecoder_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    IppDecoder_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(IppDecoder, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|    IppDecoder_Create
+---------------------------------------------------------------------*/
static BLT_Result
IppDecoder_Create(BLT_Module*              module,
                  BLT_Core*                core, 
                  BLT_ModuleParametersType parameters_type,
                  const void*              parameters, 
                  BLT_MediaNode**          object)
{
    IppDecoder* self;
    BLT_Result  result;
    
    ATX_LOG_FINE("enter");

    // initialize the IPP library
    IppStatus status = ippStaticInit();
    if (status != ippStsNoErr) {
        ATX_LOG_WARNING_1("ippStaticInit() returned %d", status);
    }

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }
    
    /* allocate memory for the object */
    self = (IppDecoder*)ATX_AllocateZeroMemory(sizeof(IppDecoder));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }
    
    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);
    
    /* construct the object */
    self->module = (IppDecoderModule*)module;
    
    /* setup the input and output ports */
    result = IppDecoder_SetupPorts(self);
    if (BLT_FAILED(result)) {
        ATX_FreeMemory(self);
        *object = NULL;
        return result;
    }
    
    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, IppDecoder, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, IppDecoder, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  IppDecoderInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  IppDecoderInput,  BLT_PacketConsumer);
    ATX_SET_INTERFACE(&self->output, IppDecoderOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, IppDecoderOutput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);
    
    return BLT_SUCCESS;
}    
    
/*----------------------------------------------------------------------
|   IppDecoderModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
IppDecoderModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    IppDecoderModule* self = ATX_SELF_EX(IppDecoderModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*     registry;
    BLT_Result        result;

    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the type id for BLT_MP4_VIDEO_ES_MIME_TYPE */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        BLT_MP4_VIDEO_ES_MIME_TYPE,
        &self->mp4_video_es_type_id);
    if (BLT_FAILED(result)) return result;
    ATX_LOG_FINE_1(BLT_MP4_VIDEO_ES_MIME_TYPE " type = %d)", self->mp4_video_es_type_id);

    /* register the type id for BLT_ISO_BASE_VIDEO_ES_MIME_TYPE */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        BLT_ISO_BASE_VIDEO_ES_MIME_TYPE,
        &self->iso_base_video_es_type_id);
    if (BLT_FAILED(result)) return result;
    ATX_LOG_FINE_1(BLT_ISO_BASE_VIDEO_ES_MIME_TYPE " type = %d)", self->iso_base_video_es_type_id);
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   IppDecoderModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
IppDecoderModule_Probe(BLT_Module*              _self, 
                       BLT_Core*                core,
                       BLT_ModuleParametersType parameters_type,
                       BLT_AnyConst             parameters,
                       BLT_Cardinal*            match)
{
    IppDecoderModule* self = ATX_SELF_EX(IppDecoderModule, BLT_BaseModule, BLT_Module);
    BLT_COMPILER_UNUSED(core);
    
    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = (BLT_MediaNodeConstructor*)parameters;

            /* the input and output protocols should be PACKET or ANY */
            if ((constructor->spec.input.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol != BLT_MEDIA_PORT_PROTOCOL_PACKET) ||
                (constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_PACKET)) {
                return BLT_FAILURE;
            }

            /* the input type should be mp4 or iso base video */
            if (constructor->spec.input.media_type->id != self->mp4_video_es_type_id &&
                constructor->spec.input.media_type->id != self->iso_base_video_es_type_id) {
                return BLT_FAILURE;
            }

            /* the output type should be unspecified, or video/raw */
            if (constructor->spec.output.media_type->id != BLT_MEDIA_TYPE_ID_VIDEO_RAW &&
                constructor->spec.output.media_type->id != BLT_MEDIA_TYPE_ID_UNKNOWN) {
                return BLT_FAILURE;
            }

            /* compute the match level */
            if (constructor->name != NULL) {
                /* we're being probed by name */
                if (ATX_StringsEqual(constructor->name, "com.bluetune.decoders.ipp")) {
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

            ATX_LOG_FINE_1("probe Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(IppDecoderModule)
    ATX_GET_INTERFACE_ACCEPT_EX(IppDecoderModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(IppDecoderModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(IppDecoderModule, IppDecoder)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(IppDecoderModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    IppDecoderModule_Attach,
    IppDecoderModule_CreateInstance,
    IppDecoderModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define IppDecoderModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(IppDecoderModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   node constructor
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_CONSTRUCTOR(IppDecoderModule, "Intel IPP Decoder", 0)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_Result 
BLT_IppDecoderModule_GetModuleObject(BLT_Module** object)
{
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    return IppDecoderModule_Create(object);
}
