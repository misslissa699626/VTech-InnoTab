/*****************************************************************
|
|   Callback Output Module
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltCallbackOutput.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketConsumer.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.outputs.callback")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);
} CallbackOutputModule;

typedef struct {
    /* base class */
    ATX_EXTENDS   (BLT_BaseMediaNode);

    /* interfaces */
    ATX_IMPLEMENTS(BLT_PacketConsumer);
    ATX_IMPLEMENTS(BLT_OutputNode);
    ATX_IMPLEMENTS(BLT_MediaPort);

    /* members */
    BLT_MediaType*      expected_media_type;
    BLT_PacketConsumer* callback_target;
} CallbackOutput;

/*----------------------------------------------------------------------
|    forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(CallbackOutputModule, BLT_Module)

ATX_DECLARE_INTERFACE_MAP(CallbackOutput, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(CallbackOutput, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(CallbackOutput, BLT_OutputNode)
ATX_DECLARE_INTERFACE_MAP(CallbackOutput, BLT_MediaPort)
ATX_DECLARE_INTERFACE_MAP(CallbackOutput, BLT_PacketConsumer)

/*----------------------------------------------------------------------
|    CallbackOutput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
CallbackOutput_PutPacket(BLT_PacketConsumer* _self,
                         BLT_MediaPacket*    packet)
{
    CallbackOutput* self = ATX_SELF(CallbackOutput, BLT_PacketConsumer);
    
    return BLT_PacketConsumer_PutPacket(self->callback_target, packet);
}

/*----------------------------------------------------------------------
|    CallbackOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
CallbackOutput_QueryMediaType(BLT_MediaPort*        _self,
                              BLT_Ordinal           index,
                              const BLT_MediaType** media_type)
{
    CallbackOutput* self = ATX_SELF(CallbackOutput, BLT_MediaPort);

    if (index == 0) {
        *media_type = self->expected_media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|    CallbackOutput_Create
+---------------------------------------------------------------------*/
static BLT_Result
CallbackOutput_Create(BLT_Module*              module,
                      BLT_Core*                core, 
                      BLT_ModuleParametersType parameters_type,
                      BLT_CString              parameters, 
                      BLT_MediaNode**          object)
{
    CallbackOutput*           self;
    ATX_Int64                 target_addr = 0;
    BLT_MediaNodeConstructor* constructor = (BLT_MediaNodeConstructor*)parameters;
    BLT_Result                result;
    
    ATX_LOG_FINE("start");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR ||
        constructor->name == NULL ||
        ATX_StringLength(constructor->name) < 16) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* check that we support pointers as integers */
    if (sizeof(void*) > sizeof(target_addr)) {
        return ATX_ERROR_NOT_SUPPORTED;
    }

    /* parse the name */
    result = ATX_ParseInteger64(constructor->name+16, &target_addr, ATX_FALSE);
    if (ATX_FAILED(result)) return result;

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(CallbackOutput));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* keep a reference to the callback target */
    self->callback_target = (BLT_PacketConsumer*)(ATX_IntPtr)target_addr;
    ATX_REFERENCE_OBJECT(self->callback_target);

    /* keep the media type info */
    BLT_MediaType_Clone(constructor->spec.input.media_type, 
                        &self->expected_media_type); 

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, CallbackOutput, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, CallbackOutput, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(self, CallbackOutput, BLT_PacketConsumer);
    ATX_SET_INTERFACE(self, CallbackOutput, BLT_OutputNode);
    ATX_SET_INTERFACE(self, CallbackOutput, BLT_MediaPort);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    CallbackOutput_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
CallbackOutput_Destroy(CallbackOutput* self)
{
    ATX_LOG_FINE("CallbackOutput::Destroy");

    /* release our target */
    ATX_RELEASE_OBJECT(self->callback_target);

    /* free the media type extensions */
    BLT_MediaType_Free(self->expected_media_type);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CallbackOutput_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
CallbackOutput_GetPortByName(BLT_MediaNode*  _self,
                             BLT_CString     name,
                             BLT_MediaPort** port)
{
    CallbackOutput* self = ATX_SELF_EX(CallbackOutput, BLT_BaseMediaNode, BLT_MediaNode);

    if (ATX_StringsEqual(name, "input")) {
        *port = &ATX_BASE(self, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|   CallbackOutput_GetStatus
+---------------------------------------------------------------------*/
BLT_METHOD
CallbackOutput_GetStatus(BLT_OutputNode*       _self, 
                         BLT_OutputNodeStatus* status)
{
    ATX_COMPILER_UNUSED(_self);

    status->media_time.seconds     = 0;
    status->media_time.nanoseconds = 0;
    status->flags                  = 0;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(CallbackOutput)
    ATX_GET_INTERFACE_ACCEPT_EX(CallbackOutput, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(CallbackOutput, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT(CallbackOutput, BLT_OutputNode)
    ATX_GET_INTERFACE_ACCEPT(CallbackOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(CallbackOutput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(CallbackOutput, "input", PACKET, IN)
ATX_BEGIN_INTERFACE_MAP(CallbackOutput, BLT_MediaPort)
    CallbackOutput_GetName,
    CallbackOutput_GetProtocol,
    CallbackOutput_GetDirection,
    CallbackOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(CallbackOutput, BLT_PacketConsumer)
    CallbackOutput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(CallbackOutput, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    CallbackOutput_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    BLT_BaseMediaNode_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|    BLT_OutputNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(CallbackOutput, BLT_OutputNode)
    CallbackOutput_GetStatus,
    NULL
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(CallbackOutput, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   CallbackOutputModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
CallbackOutputModule_Probe(BLT_Module*              self, 
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
            BLT_MediaNodeConstructor* constructor = (BLT_MediaNodeConstructor*)parameters;

            /* we need a name */
            if (constructor->name == NULL) return BLT_FAILURE;

            /* the input protocol should be PACKET and the */
            /* output protocol should be NONE              */
            if ((constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_PACKET) ||
                (constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_NONE)) {
                return BLT_FAILURE;
            }

            /* the name should be 'callback-output:<addr>' */
            if (!ATX_StringsEqualN(constructor->name, "callback-output:", 16)) {
                return BLT_FAILURE;
            }

            /* always an exact match, since we only respond to our name */
            *match = BLT_MODULE_PROBE_MATCH_EXACT;

            ATX_LOG_FINE_1("probe ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(CallbackOutputModule)
    ATX_GET_INTERFACE_ACCEPT_EX(CallbackOutputModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(CallbackOutputModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(CallbackOutputModule, CallbackOutput)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(CallbackOutputModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    CallbackOutputModule_CreateInstance,
    CallbackOutputModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define CallbackOutputModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(CallbackOutputModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_Result 
BLT_CallbackOutputModule_GetModuleObject(BLT_Module** object)
{
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    return BLT_BaseModule_Create("Callback Output", NULL, 0, 
                                 &CallbackOutputModule_BLT_ModuleInterface,
                                 &CallbackOutputModule_ATX_ReferenceableInterface,
                                 object);
}
