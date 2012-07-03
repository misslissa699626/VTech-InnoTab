/*****************************************************************
|
|   Wave Formatter Module
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
#include "BltWaveFormatter.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltByteStreamUser.h"
#include "BltByteStreamProvider.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.formatters.wave")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_UInt32 wav_type_id;
} WaveFormatterModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_OutputStreamProvider);

    /* members */
    BLT_LargeSize    size;
    BLT_PcmMediaType media_type;
} WaveFormatterInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_OutputStreamUser);

    /* members */
    BLT_MediaType     media_type;
    ATX_OutputStream* stream;
} WaveFormatterOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    WaveFormatterInput  input;
    WaveFormatterOutput output;
} WaveFormatter;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define WAVE_FORMAT_PCM 1
#define BLT_WAVE_FORMATTER_RIFF_HEADER_SIZE 44

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(WaveFormatterModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(WaveFormatter, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(WaveFormatter, ATX_Referenceable)

/*----------------------------------------------------------------------
|    WaveFormatter_WriteWavHeader
+---------------------------------------------------------------------*/
static BLT_Result
WaveFormatter_WriteWavHeader(WaveFormatter*          self, 
                             const BLT_PcmMediaType* pcm_type)
{
    unsigned char buffer[4];

    /* RIFF tag */
    ATX_OutputStream_Write(self->output.stream, "RIFF", 4, NULL);

    /* RIFF chunk size */
    ATX_BytesFromInt32Le(buffer, (ATX_Size)self->input.size + 8+16+12);
    ATX_OutputStream_Write(self->output.stream, buffer, 4, NULL);

    ATX_OutputStream_Write(self->output.stream, "WAVE", 4, NULL);

    ATX_OutputStream_Write(self->output.stream, "fmt ", 4, NULL);

    ATX_BytesFromInt32Le(buffer, 16L);
    ATX_OutputStream_Write(self->output.stream, buffer, 4, NULL);

    ATX_BytesFromInt16Le(buffer, WAVE_FORMAT_PCM);
    ATX_OutputStream_Write(self->output.stream, buffer, 2, NULL);

    /* number of channels */
    ATX_BytesFromInt16Le(buffer, (short)pcm_type->channel_count);        
    ATX_OutputStream_Write(self->output.stream, buffer, 2, NULL);

    /* sample rate */
    ATX_BytesFromInt32Le(buffer, pcm_type->sample_rate);       
    ATX_OutputStream_Write(self->output.stream, buffer, 4, NULL);

    /* bytes per second */
    ATX_BytesFromInt32Le(buffer, 
                         pcm_type->sample_rate * 
                         pcm_type->channel_count * 
                         (pcm_type->bits_per_sample/8));
    ATX_OutputStream_Write(self->output.stream, buffer, 4, NULL);

    /* alignment   */
    ATX_BytesFromInt16Le(buffer, 
                         (short)(pcm_type->channel_count * 
                                 (pcm_type->bits_per_sample/8)));     
    ATX_OutputStream_Write(self->output.stream, buffer, 2, NULL);

    /* bits per sample */
    ATX_BytesFromInt16Le(buffer, pcm_type->bits_per_sample);               
    ATX_OutputStream_Write(self->output.stream, buffer, 2, NULL);

    ATX_OutputStream_Write(self->output.stream, "data", 4, NULL);

    /* data size */
    ATX_BytesFromInt32Le(buffer, (ATX_Size)self->input.size);        
    ATX_OutputStream_Write(self->output.stream, buffer, 4, NULL);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    WaveFormatter_UpdateWavHeader
+---------------------------------------------------------------------*/
static BLT_Result
WaveFormatter_UpdateWavHeader(WaveFormatter* self)
{
    BLT_Result    result;
    unsigned char buffer[4];

    ATX_LOG_FINER_1("WaveFormatter::UpdateWavHeader - size = %d", self->input.size);

    result = ATX_OutputStream_Seek(self->output.stream, 4);
    if (BLT_FAILED(result)) return result;
    ATX_BytesFromInt32Le(buffer, (ATX_Size)self->input.size + 8+16+12);
    ATX_OutputStream_Write(self->output.stream, buffer, 4, NULL);

    result = ATX_OutputStream_Seek(self->output.stream, 40);
    if (BLT_FAILED(result)) return result;

    ATX_BytesFromInt32Le(buffer, (ATX_Size)self->input.size);        
    ATX_OutputStream_Write(self->output.stream, buffer, 4, NULL);

    ATX_LOG_FINER("WaveFormatter::UpdateWavHeader - updated");

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    WaveFormatterInputPort_GetStream
+---------------------------------------------------------------------*/
BLT_METHOD
WaveFormatterInput_GetStream(BLT_OutputStreamProvider* _self,
                             ATX_OutputStream**        stream,
                             const BLT_MediaType*      media_type)
{
    WaveFormatter* self = ATX_SELF_M(input, WaveFormatter, BLT_OutputStreamProvider);

    /* check that we have a stream */
    if (self->output.stream == NULL) return BLT_ERROR_PORT_HAS_NO_STREAM;

    /* return our output stream */
    *stream = self->output.stream;
    ATX_REFERENCE_OBJECT(*stream);

    /* we're providing the stream, but we *receive* the type */
    if (media_type) {
        if (media_type->id != BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
            return BLT_ERROR_INVALID_MEDIA_TYPE;
        } else {
            /* copy the input type parameters */
            self->input.media_type = *(const BLT_PcmMediaType*)media_type;
        }
    }

    /* write a header unless the output stream already has some data */
    /* (this might be due to the fact that we're writing more than   */
    /*  one input stream into the same output stream                 */
    {
        ATX_Position where = 0;
        ATX_OutputStream_Tell(self->output.stream, &where);
        if (where == 0) { 
            WaveFormatter_WriteWavHeader(self, &self->input.media_type);
        }
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    WaveFormatterInputPort_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
WaveFormatterInput_QueryMediaType(BLT_MediaPort*        _self,
                                  BLT_Ordinal           index,
                                  const BLT_MediaType** media_type)
{
    WaveFormatter* self = ATX_SELF_M(input, WaveFormatter, BLT_MediaPort);

    if (index == 0) {
        *media_type = (const BLT_MediaType*)&self->input.media_type;
        return BLT_SUCCESS;
    } else {
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(WaveFormatterInput)
    ATX_GET_INTERFACE_ACCEPT(WaveFormatterInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(WaveFormatterInput, BLT_OutputStreamProvider)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_OutputStreamProvider interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(WaveFormatterInput, BLT_OutputStreamProvider)
    WaveFormatterInput_GetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(WaveFormatterInput, 
                                         "input",
                                         STREAM_PUSH,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(WaveFormatterInput, BLT_MediaPort)
    WaveFormatterInput_GetName,
    WaveFormatterInput_GetProtocol,
    WaveFormatterInput_GetDirection,
    WaveFormatterInput_QueryMediaType
};


/*----------------------------------------------------------------------
|    WaveFormatterOutput_SetStream
+---------------------------------------------------------------------*/
BLT_METHOD
WaveFormatterOutput_SetStream(BLT_OutputStreamUser* _self,
                              ATX_OutputStream*     stream)
{
    WaveFormatter* self = ATX_SELF_M(output, WaveFormatter, BLT_OutputStreamUser);

    /* keep a reference to the stream */
    self->output.stream = stream;
    ATX_REFERENCE_OBJECT(stream);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    WaveFormatterOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
WaveFormatterOutput_QueryMediaType(BLT_MediaPort*        _self,
                                   BLT_Ordinal           index,
                                   const BLT_MediaType** media_type)
{
    WaveFormatter* self = ATX_SELF_M(output, WaveFormatter, BLT_MediaPort);
    if (index == 0) {
        *media_type = &self->output.media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(WaveFormatterOutput)
    ATX_GET_INTERFACE_ACCEPT(WaveFormatterOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(WaveFormatterOutput, BLT_OutputStreamUser)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(WaveFormatterOutput,
                                         "output",
                                         STREAM_PUSH,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(WaveFormatterOutput, BLT_MediaPort)
    WaveFormatterOutput_GetName,
    WaveFormatterOutput_GetProtocol,
    WaveFormatterOutput_GetDirection,
    WaveFormatterOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_OutputStreamUser interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(WaveFormatterOutput, BLT_OutputStreamUser)
    WaveFormatterOutput_SetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    WaveFormatter_Create
+---------------------------------------------------------------------*/
static BLT_Result
WaveFormatter_Create(BLT_Module*              module,
                     BLT_Core*                core, 
                     BLT_ModuleParametersType parameters_type,
                     BLT_CString              parameters, 
                     BLT_MediaNode**          object)
{
    WaveFormatter* self;

    ATX_LOG_FINE("WaveFormatter::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(WaveFormatter));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* setup the input media type */
    BLT_PcmMediaType_Init(&self->input.media_type);
    self->input.media_type.sample_format = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_LE;

    /* setup the output media type */
    BLT_MediaType_Init(&self->output.media_type, 
                       ((WaveFormatterModule*)module)->wav_type_id);

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, WaveFormatter, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, WaveFormatter, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  WaveFormatterInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  WaveFormatterInput,  BLT_OutputStreamProvider);
    ATX_SET_INTERFACE(&self->output, WaveFormatterOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, WaveFormatterOutput, BLT_OutputStreamUser);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    WaveFormatter_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
WaveFormatter_Destroy(WaveFormatter* self)
{
    ATX_LOG_FINE("WaveFormatter::Destroy");

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    WaveFormatter_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
WaveFormatter_Deactivate(BLT_MediaNode* _self)
{
    WaveFormatter* self = ATX_SELF_EX(WaveFormatter, BLT_BaseMediaNode, BLT_MediaNode);

    ATX_LOG_FINER("WaveFormatter::Deactivate");

    /* update the header if needed */
    if (self->output.stream) {
        ATX_Position where = 0;
        ATX_OutputStream_Tell(self->output.stream, &where);
        self->input.size = where;
        if (self->input.size >= BLT_WAVE_FORMATTER_RIFF_HEADER_SIZE) {
            self->input.size -= BLT_WAVE_FORMATTER_RIFF_HEADER_SIZE;
        }

        /* update the header */
        WaveFormatter_UpdateWavHeader(self);

        /* set the stream back to its original position */
        ATX_OutputStream_Seek(self->output.stream, where);
    }

    /* release the output stream */
    ATX_RELEASE_OBJECT(self->output.stream);

    /* call the base class method */
    BLT_BaseMediaNode_Deactivate(_self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WaveFormatter_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
WaveFormatter_GetPortByName(BLT_MediaNode*  _self,
                            BLT_CString     name,
                            BLT_MediaPort** port)
{
    WaveFormatter* self = ATX_SELF_EX(WaveFormatter, BLT_BaseMediaNode, BLT_MediaNode);

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
|   standard GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(WaveFormatter)
    ATX_GET_INTERFACE_ACCEPT_EX(WaveFormatter, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(WaveFormatter, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(WaveFormatter, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    WaveFormatter_GetPortByName,
    BLT_BaseMediaNode_Activate,
    WaveFormatter_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    BLT_BaseMediaNode_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(WaveFormatter, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   WaveFormatterModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
WaveFormatterModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    WaveFormatterModule* self = ATX_SELF_EX(WaveFormatterModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*        registry;
    BLT_Result           result;

    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the .wav file extensions */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".wav",
                                            "audio/wav");
    if (BLT_FAILED(result)) return result;

    /* register the "audio/wav" type */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/wav",
        &self->wav_type_id);
    if (BLT_FAILED(result)) return result;
    
    ATX_LOG_FINE_1("WaveFormatterModule::Attach (audio/wav type = %d)", self->wav_type_id);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WaveFormatterModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
WaveFormatterModule_Probe(BLT_Module*              _self, 
                          BLT_Core*                core,
                          BLT_ModuleParametersType parameters_type,
                          BLT_AnyConst             parameters,
                          BLT_Cardinal*            match)
{
    WaveFormatterModule* self = ATX_SELF_EX(WaveFormatterModule, BLT_BaseModule, BLT_Module);
    BLT_COMPILER_UNUSED(core);

    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

            /* the input protocol should be STREAM_PUSH and the */
            /* output protocol should be STREAM_PUSH            */
            if ((constructor->spec.input.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_STREAM_PUSH) ||
                (constructor->spec.output.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_STREAM_PUSH)) {
                return BLT_FAILURE;
            }

            /* the input type should be audio/pcm */
            if (constructor->spec.input.media_type->id !=
                BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
                return BLT_FAILURE;
            }

            /* compute the match level */
            if (constructor->name != NULL) {
                /* we're being probed by name */
                if (ATX_StringsEqual(constructor->name, "WaveFormatter")) {
                    /* our name */
                    *match = BLT_MODULE_PROBE_MATCH_EXACT;
                } else {
                    /* not our name */
                    return BLT_FAILURE;
                }
            } else {
                /* we're probed by protocol/type specs only */

                /* the output type should be audio/wav */
                if (constructor->spec.output.media_type->id !=
                    self->wav_type_id) {
                    return BLT_FAILURE;
                }

                *match = BLT_MODULE_PROBE_MATCH_MAX - 10;
            }

            ATX_LOG_FINE_1("WaveFormatterModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(WaveFormatterModule)
    ATX_GET_INTERFACE_ACCEPT_EX(WaveFormatterModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(WaveFormatterModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(WaveFormatterModule, WaveFormatter)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(WaveFormatterModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    WaveFormatterModule_Attach,
    WaveFormatterModule_CreateInstance,
    WaveFormatterModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define WaveFormatterModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(WaveFormatterModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(WaveFormatterModule,
                                         "WAVE Formatter",
                                         "com.axiosys.formatter.wave",
                                         "1.1.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
