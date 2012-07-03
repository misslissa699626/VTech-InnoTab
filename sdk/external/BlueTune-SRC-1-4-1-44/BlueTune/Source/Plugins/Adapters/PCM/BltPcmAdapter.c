/*****************************************************************
|
|   PCM Adapter Module
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
#include "BltCore.h"
#include "BltPcmAdapter.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltPacketConsumer.h"
#include "BltStream.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.adapters.pcm")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef BLT_BaseModule PcmAdapterModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketConsumer);
} PcmAdapterInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    BLT_PcmMediaType pcm_type;
    BLT_MediaPacket* packet;
} PcmAdapterOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    PcmAdapterInput  input;
    PcmAdapterOutput output;
} PcmAdapter;

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(PcmAdapterModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(PcmAdapter, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(PcmAdapter, ATX_Referenceable)

/*----------------------------------------------------------------------
|    PcmAdapterInput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
PcmAdapterInput_PutPacket(BLT_PacketConsumer* _self,
                          BLT_MediaPacket*    packet)
{
    PcmAdapter* self = ATX_SELF_M(input, PcmAdapter, BLT_PacketConsumer);
    BLT_Result  result;

    /* transform the packet data */
    result =  BLT_Pcm_ConvertMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
                                         packet, 
                                         &self->output.pcm_type, 
                                         &self->output.packet);
    if (BLT_FAILED(result)) {
        ATX_LOG_WARNING_1("PcmAdapterInput::PutPacket - failed to convert PCM (%d)", result);
    }

    return result;
}

/*----------------------------------------------------------------------
|   PcmAdapterInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
PcmAdapterInput_QueryMediaType(BLT_MediaPort*         self,
                               BLT_Ordinal            index,
                               const BLT_MediaType**  media_type)
{
    BLT_COMPILER_UNUSED(self);
    if (index == 0) {
        *media_type = &BLT_GenericPcmMediaType;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(PcmAdapterInput)
    ATX_GET_INTERFACE_ACCEPT(PcmAdapterInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(PcmAdapterInput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(PcmAdapterInput, BLT_PacketConsumer)
    PcmAdapterInput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(PcmAdapterInput,
                                         "input",
                                         PACKET,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(PcmAdapterInput, BLT_MediaPort)
    PcmAdapterInput_GetName,
    PcmAdapterInput_GetProtocol,
    PcmAdapterInput_GetDirection,
    PcmAdapterInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    PcmAdapterOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
PcmAdapterOutput_GetPacket(BLT_PacketProducer* _self,
                           BLT_MediaPacket**   packet)
{
    PcmAdapter* self = ATX_SELF_M(output, PcmAdapter, BLT_PacketProducer);

    if (self->output.packet) {
        *packet = self->output.packet;
        self->output.packet = NULL;
        return BLT_SUCCESS;
    } else {
        *packet = NULL;
        return BLT_ERROR_PORT_HAS_NO_DATA;
    }
}

/*----------------------------------------------------------------------
|   PcmAdapterOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
PcmAdapterOutput_QueryMediaType(BLT_MediaPort*        self,
                                BLT_Ordinal           index,
                                const BLT_MediaType** media_type)
{
    BLT_COMPILER_UNUSED(self);
    if (index == 0) {
        *media_type = &BLT_GenericPcmMediaType;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(PcmAdapterOutput)
    ATX_GET_INTERFACE_ACCEPT(PcmAdapterOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(PcmAdapterOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(PcmAdapterOutput,
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(PcmAdapterOutput, BLT_MediaPort)
    PcmAdapterOutput_GetName,
    PcmAdapterOutput_GetProtocol,
    PcmAdapterOutput_GetDirection,
    PcmAdapterOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(PcmAdapterOutput, BLT_PacketProducer)
    PcmAdapterOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    PcmAdapter_Create
+---------------------------------------------------------------------*/
static BLT_Result
PcmAdapter_Create(BLT_Module*              module,
                  BLT_Core*                core, 
                  BLT_ModuleParametersType parameters_type,
                  BLT_AnyConst             parameters, 
                  BLT_MediaNode**          object)
{
    BLT_MediaNodeConstructor* constructor = (BLT_MediaNodeConstructor*)parameters;
    PcmAdapter*               self;

    ATX_LOG_FINE("PcmAdapter::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(PcmAdapter));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* check the media type */
    if (constructor->spec.output.media_type->id != BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* construct the object */
    self->output.pcm_type = *(BLT_PcmMediaType*)constructor->spec.output.media_type;

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, PcmAdapter, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, PcmAdapter, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  PcmAdapterInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  PcmAdapterInput,  BLT_PacketConsumer);
    ATX_SET_INTERFACE(&self->output, PcmAdapterOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, PcmAdapterOutput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    PcmAdapter_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
PcmAdapter_Destroy(PcmAdapter* self)
{ 
    ATX_LOG_FINE("PcmAdapter::Destroy");

    /* release any input packet we may hold */
    if (self->output.packet) {
        BLT_MediaPacket_Release(self->output.packet);
    }

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory((void*)self);

    return BLT_SUCCESS;
}
                    
/*----------------------------------------------------------------------
|   PcmAdapter_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
PcmAdapter_GetPortByName(BLT_MediaNode*  _self,
                         BLT_CString     name,
                         BLT_MediaPort** port)
{
    PcmAdapter* self = ATX_SELF_EX(PcmAdapter, BLT_BaseMediaNode, BLT_MediaNode);

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
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(PcmAdapter)
    ATX_GET_INTERFACE_ACCEPT_EX(PcmAdapter, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(PcmAdapter, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(PcmAdapter, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    PcmAdapter_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    BLT_BaseMediaNode_Seek
};

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(PcmAdapter, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   PcmAdapterModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
PcmAdapterModule_Probe(BLT_Module*              self,  
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

            /* compute match based on specified name */
            if (constructor->name == NULL) {
                *match = BLT_MODULE_PROBE_MATCH_DEFAULT;

                /* the input protocol should be PACKET */
                if (constructor->spec.input.protocol != 
                    BLT_MEDIA_PORT_PROTOCOL_PACKET) {
                    return BLT_FAILURE;
                }

                /* output protocol should be PACKET */
                if (constructor->spec.output.protocol != 
                    BLT_MEDIA_PORT_PROTOCOL_PACKET) {
                    return BLT_FAILURE;
                }

                /* check that the in and out formats are supported */
                if (!BLT_Pcm_CanConvert(constructor->spec.input.media_type, 
                                        constructor->spec.output.media_type)) {
                    return BLT_FAILURE;
                }
            } else {
                /* if a name is specified, it needs to match exactly */
                if (!ATX_StringsEqual(constructor->name, "PcmAdapter")) {
                    return BLT_FAILURE;
                } else {
                    *match = BLT_MODULE_PROBE_MATCH_EXACT;
                }

                /* the input protocol should be PACKET or ANY */
                if (constructor->spec.input.protocol !=
                    BLT_MEDIA_PORT_PROTOCOL_ANY &&
                    constructor->spec.input.protocol != 
                    BLT_MEDIA_PORT_PROTOCOL_PACKET) {
                    return BLT_FAILURE;
                }

                /* output protocol should be PACKET or ANY */
                if ((constructor->spec.output.protocol !=
                    BLT_MEDIA_PORT_PROTOCOL_ANY &&
                    constructor->spec.output.protocol != 
                    BLT_MEDIA_PORT_PROTOCOL_PACKET)) {
                    return BLT_FAILURE;
                }

                /* check that the in and out formats are supported */
                if (!BLT_Pcm_CanConvert(constructor->spec.input.media_type, 
                                        constructor->spec.output.media_type)) {
                    return BLT_FAILURE;
                }
            }

            ATX_LOG_FINE_1("PcmAdapterModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(PcmAdapterModule)
    ATX_GET_INTERFACE_ACCEPT(PcmAdapterModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT(PcmAdapterModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(PcmAdapterModule, PcmAdapter)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(PcmAdapterModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    PcmAdapterModule_CreateInstance,
    PcmAdapterModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define PcmAdapterModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE(PcmAdapterModule, reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(PcmAdapterModule,
                                         "PCM Adapter",
                                         "com.axiosys.adapter.pcm",
                                         "1.1.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
