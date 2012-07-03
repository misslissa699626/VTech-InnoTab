/*****************************************************************
|
|      Cdda: BltCddaInput.c
|
|      Cdda Input Module
|
|      (c) 2002-2003 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltCddaInput.h"
#include "BltCddaDevice.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltModule.h"
#include "BltByteStreamProvider.h"
#include "BltStream.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.inputs.cdda")

/*----------------------------------------------------------------------
|    forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(CddaInputModule, BLT_Module)

ATX_DECLARE_INTERFACE_MAP(CddaInput, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(CddaInput, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(CddaInput, BLT_MediaPort)
ATX_DECLARE_INTERFACE_MAP(CddaInput, BLT_InputStreamProvider)

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);
} CddaInputModule;

typedef struct {
    /* interfaces */
    ATX_EXTENDS(BLT_BaseMediaNode);
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamProvider);

    /* members */
    BLT_PcmMediaType media_type;
    BLT_CddaDevice*  device;
    BLT_Ordinal      track_index;
    ATX_InputStream* track;
} CddaInput;

/*----------------------------------------------------------------------
|    CddaInput_Create
+---------------------------------------------------------------------*/
static BLT_Result
CddaInput_Create(BLT_Module*              module,
                 BLT_Core*                core, 
                 BLT_ModuleParametersType parameters_type,
                 BLT_CString              parameters, 
                 BLT_MediaNode**          object)
{
    CddaInput*                input;
    BLT_MediaNodeConstructor* constructor = 
        (BLT_MediaNodeConstructor*)parameters;

    ATX_LOG_FINE("CddaInput::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR ||
        constructor->name == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* check name length */
    if (ATX_StringLength(constructor->name) < 6) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    input = ATX_AllocateZeroMemory(sizeof(CddaInput));
    if (input == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(input, BLT_BaseMediaNode), module, core);

    /* setup the media type */
    BLT_PcmMediaType_Init(&input->media_type);
	input->media_type.sample_rate     = 44100;
    input->media_type.channel_count   = 2;
    input->media_type.channel_mask    = 0;
    input->media_type.bits_per_sample = 16;
    input->media_type.sample_format   = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_LE;

    /* parse the name */
    {
        const char* c_index = &constructor->name[5];
        while (*c_index >= '0' && *c_index <= '9') {
            input->track_index = 10*input->track_index + *c_index++ -'0';
        }
    }
    ATX_LOG_FINE_1("CddaInput::Create - track index = %d", input->track_index);

    /* construct reference */
    ATX_SET_INTERFACE_EX(input, CddaInput, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(input, CddaInput, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE   (input, CddaInput, BLT_MediaPort);
    ATX_SET_INTERFACE   (input, CddaInput, BLT_InputStreamProvider);
    *object = &ATX_BASE_EX(input, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    CddaInput_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
CddaInput_Destroy(CddaInput* self)
{
    ATX_LOG_FINE("CddaInput::Destroy");

    /* release the track */
    ATX_RELEASE_OBJECT(self->track);
    
    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    CddaInput_Activate
+---------------------------------------------------------------------*/
BLT_METHOD
CddaInput_Activate(BLT_MediaNode* _self, BLT_Stream* stream)
{
    CddaInput* self = ATX_SELF_EX(CddaInput, BLT_BaseMediaNode, BLT_MediaNode);
    BLT_CddaTrackInfo track_info;
    BLT_StreamInfo    stream_info;
    BLT_Result        result;

    ATX_LOG_FINER("CddaInput::Activate");

    /* keep the stream as our context */
    ATX_BASE(self, BLT_BaseMediaNode).context = stream;

    /* open the device */
    result = BLT_CddaDevice_Create(NULL, &self->device);
    if (BLT_FAILED(result)) return result;

    /* get track info */
    result = BLT_CddaDevice_GetTrackInfo(self->device, 
                                         self->track_index,
                                         &track_info);
    if (BLT_FAILED(result)) return result;

    /* check that track is audio */
    if (track_info.type != BLT_CDDA_TRACK_TYPE_AUDIO) {
      return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* create a track object to read from */
    result = BLT_CddaTrack_Create(self->device, 
                                  self->track_index,
                                  &self->track);
    if (BLT_FAILED(result)) return result;

    /* start with no info */
    stream_info.mask = 0;

    /* stream size */
    stream_info.size = track_info.duration.frames * BLT_CDDA_FRAME_SIZE;
    stream_info.mask |= BLT_STREAM_INFO_MASK_SIZE;

    /* stream duration */
    stream_info.duration = 
        (track_info.duration.frames*1000)/
        BLT_CDDA_FRAMES_PER_SECOND;
    stream_info.mask |= BLT_STREAM_INFO_MASK_DURATION;

    /* sample rate */
    stream_info.sample_rate = 44100;
    stream_info.mask |= BLT_STREAM_INFO_MASK_SAMPLE_RATE;

    /* channel count */
    stream_info.channel_count = 2;
    stream_info.mask |= BLT_STREAM_INFO_MASK_CHANNEL_COUNT;

    /* bitrates */
    stream_info.nominal_bitrate = 8*44100*4;
    stream_info.mask |= BLT_STREAM_INFO_MASK_NOMINAL_BITRATE;
    stream_info.average_bitrate = 8*44100*4;
    stream_info.mask |= BLT_STREAM_INFO_MASK_AVERAGE_BITRATE;
    stream_info.instant_bitrate = 8*44100*4;
    stream_info.mask |= BLT_STREAM_INFO_MASK_INSTANT_BITRATE;

    stream_info.data_type = "PCM";
    stream_info.mask |= BLT_STREAM_INFO_MASK_DATA_TYPE;

    /* notify the stream */
    BLT_Stream_SetInfo(stream, &stream_info);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    CddaInput_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
CddaInput_Deactivate(BLT_MediaNode* _self)
{
    CddaInput* self = ATX_SELF_EX(CddaInput, BLT_BaseMediaNode, BLT_MediaNode);

    ATX_LOG_FINER("CddaInput::Deactivate");

    /* call the base class method */
    BLT_BaseMediaNode_Deactivate(_self);

    /* release the track */
    ATX_RELEASE_OBJECT(self->track);

    /* close device */
    ATX_DESTROY_OBJECT(self->device);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CddaInput_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
CddaInput_GetPortByName(BLT_MediaNode*  _self,
                        BLT_CString     name,
                        BLT_MediaPort** port)
{
    CddaInput* self = ATX_SELF_EX(CddaInput, BLT_BaseMediaNode, BLT_MediaNode);
    if (ATX_StringsEqual(name, "output")) {
        /* we implement the BLT_MediaPort interface ourselves */
        *port = &ATX_BASE(self, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|    CddaInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
CddaInput_QueryMediaType(BLT_MediaPort*        _self,
                         BLT_Ordinal           index,
                         const BLT_MediaType** media_type)
{
    CddaInput* self = ATX_SELF(CddaInput, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = &self->media_type.base;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   CddaInput_GetStream
+---------------------------------------------------------------------*/
BLT_METHOD
CddaInput_GetStream(BLT_InputStreamProvider* _self,
                    ATX_InputStream**        stream)
{
    CddaInput* self = ATX_SELF(CddaInput, BLT_InputStreamProvider);

    /* return a referebce to the track stream */
    if (self->track) ATX_REFERENCE_OBJECT(self->track);
    *stream = self->track;
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(CddaInput)
    ATX_GET_INTERFACE_ACCEPT_EX(CddaInput, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(CddaInput, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT   (CddaInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT   (CddaInput, BLT_InputStreamProvider)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(CddaInput, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    CddaInput_GetPortByName,
    CddaInput_Activate,
    CddaInput_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    BLT_BaseMediaNode_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(CddaInput, 
                                         "output", 
                                         STREAM_PULL, 
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(CddaInput, BLT_MediaPort)
    CddaInput_GetName,
    CddaInput_GetProtocol,
    CddaInput_GetDirection,
    CddaInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_InputStreamProvider interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(CddaInput, BLT_InputStreamProvider)
    CddaInput_GetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(CddaInput, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|       CddaInputModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
CddaInputModule_Probe(BLT_Module*              self, 
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

            /* we need a cdda name */
            if (constructor->name == NULL) return BLT_FAILURE;

            /* the input protocol should be NONE, and the output */
            /* protocol should be STREAM_PULL                    */
            if ((constructor->spec.input.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_NONE) ||
                (constructor->spec.output.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL)) {
                return BLT_FAILURE;
            }

            /* check the name */
            if (ATX_StringsEqualN(constructor->name, "cdda:", 5)) {
                /* this is an exact match for us */
                *match = BLT_MODULE_PROBE_MATCH_EXACT;
            } else {
                /* not us */
                return BLT_FAILURE;
            }

            ATX_LOG_FINE_1("CddaInputModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(CddaInputModule)
    ATX_GET_INTERFACE_ACCEPT_EX(CddaInputModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(CddaInputModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(CddaInputModule, CddaInput)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(CddaInputModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    CddaInputModule_CreateInstance,
    CddaInputModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define CddaInputModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(CddaInputModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|       module object
+---------------------------------------------------------------------*/
BLT_Result 
BLT_CddaInputModule_GetModuleObject(BLT_Module** object)
{
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    return BLT_BaseModule_Create("CDDA Input", NULL, 0,
                                 &CddaInputModule_BLT_ModuleInterface,
                                 &CddaInputModule_ATX_ReferenceableInterface,
                                 object);
}
