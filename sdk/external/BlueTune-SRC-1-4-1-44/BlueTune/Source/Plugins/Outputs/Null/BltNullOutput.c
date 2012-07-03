/*****************************************************************
|
|   Null Output Module
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
#include "BltNullOutput.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPacketConsumer.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.outputs.null")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);
} NullOutputModule;

typedef struct {
    /* base class */
    ATX_EXTENDS   (BLT_BaseMediaNode);

    /* interfaces */
    ATX_IMPLEMENTS(BLT_PacketConsumer);
    ATX_IMPLEMENTS(BLT_MediaPort);

    /* members */
    BLT_MediaType* expected_media_type;
} NullOutput;

/*----------------------------------------------------------------------
|    forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(NullOutputModule, BLT_Module)

ATX_DECLARE_INTERFACE_MAP(NullOutput, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(NullOutput, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(NullOutput, BLT_MediaPort)
ATX_DECLARE_INTERFACE_MAP(NullOutput, BLT_PacketConsumer)

/*----------------------------------------------------------------------
|    NullOutput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
NullOutput_PutPacket(BLT_PacketConsumer* _self,
                     BLT_MediaPacket*    packet)
{
    NullOutput*          self = ATX_SELF(NullOutput, BLT_PacketConsumer);
    const BLT_MediaType* media_type;

    /* check the media type */
    BLT_MediaPacket_GetMediaType(packet, &media_type);
    if (self->expected_media_type->id != BLT_MEDIA_TYPE_ID_UNKNOWN &&
        self->expected_media_type->id != media_type->id) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    NullOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
NullOutput_QueryMediaType(BLT_MediaPort*        _self,
                          BLT_Ordinal           index,
                          const BLT_MediaType** media_type)
{
    NullOutput* self = ATX_SELF(NullOutput, BLT_MediaPort);

    if (index == 0) {
        *media_type = self->expected_media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|    NullOutput_Create
+---------------------------------------------------------------------*/
static BLT_Result
NullOutput_Create(BLT_Module*              module,
                  BLT_Core*                core, 
                  BLT_ModuleParametersType parameters_type,
                  BLT_CString              parameters, 
                  BLT_MediaNode**          object)
{
    NullOutput*               self;
    BLT_MediaNodeConstructor* constructor = (BLT_MediaNodeConstructor*)parameters;

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(NullOutput));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* construct the object */
    BLT_MediaType_Clone(constructor->spec.input.media_type, &self->expected_media_type);

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, NullOutput, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, NullOutput, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(self, NullOutput, BLT_PacketConsumer);
    ATX_SET_INTERFACE(self, NullOutput, BLT_MediaPort);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    NullOutput_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
NullOutput_Destroy(NullOutput* self)
{
    /* free the media type extensions */
    BLT_MediaType_Free(self->expected_media_type);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NullOutput_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
NullOutput_GetPortByName(BLT_MediaNode*  _self,
                         BLT_CString     name,
                         BLT_MediaPort** port)
{
    NullOutput* self = ATX_SELF_EX(NullOutput, BLT_BaseMediaNode, BLT_MediaNode);

    if (ATX_StringsEqual(name, "input")) {
        *port = &ATX_BASE(self, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(NullOutput)
    ATX_GET_INTERFACE_ACCEPT_EX(NullOutput, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(NullOutput, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT(NullOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(NullOutput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(NullOutput, "input", PACKET, IN)
ATX_BEGIN_INTERFACE_MAP(NullOutput, BLT_MediaPort)
    NullOutput_GetName,
    NullOutput_GetProtocol,
    NullOutput_GetDirection,
    NullOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(NullOutput, BLT_PacketConsumer)
    NullOutput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(NullOutput, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    NullOutput_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    BLT_BaseMediaNode_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(NullOutput, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   NullOutputModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
NullOutputModule_Probe(BLT_Module*              self, 
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

            /* the input protocol should be PACKET and the */
            /* output protocol should be NONE              */
            if ((constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_PACKET) ||
                (constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_NONE)) {
                return BLT_FAILURE;
            }

            /* the name should be 'null' */
            if (constructor->name == NULL ||
                !ATX_StringsEqual(constructor->name, "null")) {
                return BLT_FAILURE;
            }

            /* always an exact match, since we only respond to our name */
            *match = BLT_MODULE_PROBE_MATCH_EXACT;

            ATX_LOG_FINE_1("match score = %d", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(NullOutputModule)
    ATX_GET_INTERFACE_ACCEPT_EX(NullOutputModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(NullOutputModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(NullOutputModule, NullOutput)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(NullOutputModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    NullOutputModule_CreateInstance,
    NullOutputModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define NullOutputModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(NullOutputModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(NullOutputModule,
                                         "Null Output",
                                         "com.axiosys.output.null",
                                         "1.0.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
