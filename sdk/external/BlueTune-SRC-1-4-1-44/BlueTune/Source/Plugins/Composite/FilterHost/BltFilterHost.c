/*****************************************************************
|
|   Filter Host Module
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
#include "BltFilterHost.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltPacketConsumer.h"
#include "BltStream.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.composite.filter-host")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef BLT_BaseModule FilterHostModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketConsumer);
} FilterHostInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    BLT_PcmMediaType pcm_type;
    BLT_MediaPacket* packet;
} FilterHostOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    FilterHostInput  input;
    FilterHostOutput output;
} FilterHost;

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(FilterHostModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(FilterHost, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(FilterHost, ATX_Referenceable)

/*----------------------------------------------------------------------
|    FilterHostInput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
FilterHostInput_PutPacket(BLT_PacketConsumer* _self,
                          BLT_MediaPacket*    packet)
{
    FilterHost* self = ATX_SELF_M(input, FilterHost, BLT_PacketConsumer);
    
    /* transform the packet data */
    return BLT_Pcm_ConvertMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
                                      packet, 
                                      &self->output.pcm_type, 
                                      &self->output.packet);
}

/*----------------------------------------------------------------------
|   FilterHostInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
FilterHostInput_QueryMediaType(BLT_MediaPort*         self,
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(FilterHostInput)
    ATX_GET_INTERFACE_ACCEPT(FilterHostInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(FilterHostInput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(FilterHostInput, BLT_PacketConsumer)
    FilterHostInput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(FilterHostInput,
                                         "input",
                                         PACKET,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(FilterHostInput, BLT_MediaPort)
    FilterHostInput_GetName,
    FilterHostInput_GetProtocol,
    FilterHostInput_GetDirection,
    FilterHostInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    FilterHostOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
FilterHostOutput_GetPacket(BLT_PacketProducer* _self,
                           BLT_MediaPacket**   packet)
{
    FilterHost* self = ATX_SELF_M(output, FilterHost, BLT_PacketProducer);

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
|   FilterHostOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
FilterHostOutput_QueryMediaType(BLT_MediaPort*        self,
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(FilterHostOutput)
    ATX_GET_INTERFACE_ACCEPT(FilterHostOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(FilterHostOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(FilterHostOutput,
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(FilterHostOutput, BLT_MediaPort)
    FilterHostOutput_GetName,
    FilterHostOutput_GetProtocol,
    FilterHostOutput_GetDirection,
    FilterHostOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(FilterHostOutput, BLT_PacketProducer)
    FilterHostOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    FilterHost_Create
+---------------------------------------------------------------------*/
static BLT_Result
FilterHost_Create(BLT_Module*              module,
                  BLT_Core*                core, 
                  BLT_ModuleParametersType parameters_type,
                  BLT_AnyConst             parameters, 
                  BLT_MediaNode**          object)
{
    BLT_MediaNodeConstructor* constructor = (BLT_MediaNodeConstructor*)parameters;
    FilterHost*               self;

    ATX_LOG_FINE("FilterHost::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(FilterHost));
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
    ATX_SET_INTERFACE_EX(self, FilterHost, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, FilterHost, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  FilterHostInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  FilterHostInput,  BLT_PacketConsumer);
    ATX_SET_INTERFACE(&self->output, FilterHostOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, FilterHostOutput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    FilterHost_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
FilterHost_Destroy(FilterHost* self)
{ 
    ATX_LOG_FINE("FilterHost::Destroy");

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
|   FilterHost_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
FilterHost_GetPortByName(BLT_MediaNode*  _self,
                         BLT_CString     name,
                         BLT_MediaPort** port)
{
    FilterHost* self = ATX_SELF_EX(FilterHost, BLT_BaseMediaNode, BLT_MediaNode);

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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(FilterHost)
    ATX_GET_INTERFACE_ACCEPT_EX(FilterHost, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(FilterHost, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(FilterHost, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    FilterHost_GetPortByName,
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
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(FilterHost, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   FilterHostModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
FilterHostModule_Probe(BLT_Module*              self,  
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
                /* if a name is a specified, it needs to match exactly */
                if (!ATX_StringsEqual(constructor->name, "FilterHost")) {
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

            ATX_LOG_FINE_1("FilterHostModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(FilterHostModule)
    ATX_GET_INTERFACE_ACCEPT(FilterHostModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT(FilterHostModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(FilterHostModule, FilterHost)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(FilterHostModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    FilterHostModule_CreateInstance,
    FilterHostModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define FilterHostModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE(FilterHostModule, reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_Result 
BLT_FilterHostModule_GetModuleObject(BLT_Module** object)
{
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    return BLT_BaseModule_Create("Filter Hostr", NULL, 0, 
                                 &FilterHostModule_BLT_ModuleInterface, 
                                 &FilterHostModule_ATX_ReferenceableInterface, 
                                 object);
}
