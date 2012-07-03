/*****************************************************************
|
|   BlueTune - Callback Input Module
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
#include "BltCallbackInput.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltModule.h"
#include "BltByteStreamProvider.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.inputs.callback")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);
} CallbackInputModule;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);
    
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamProvider);

    /* members */
    ATX_InputStream* stream;
    BLT_MediaType*   media_type;
} CallbackInput;

/*----------------------------------------------------------------------
|    forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(CallbackInputModule, BLT_Module)

ATX_DECLARE_INTERFACE_MAP(CallbackInput, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(CallbackInput, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(CallbackInput, BLT_MediaPort)
ATX_DECLARE_INTERFACE_MAP(CallbackInput, BLT_InputStreamProvider)
static BLT_Result CallbackInput_Destroy(CallbackInput* self);

/*----------------------------------------------------------------------
|    CallbackInput_Create
+---------------------------------------------------------------------*/
static BLT_Result
CallbackInput_Create(BLT_Module*              module,
                     BLT_Core*                core, 
                     BLT_ModuleParametersType parameters_type,
                     BLT_AnyConst             parameters, 
                     BLT_MediaNode**          object)
{
    CallbackInput*            input;
    BLT_MediaNodeConstructor* constructor = (BLT_MediaNodeConstructor*)parameters;
    ATX_Int64                 stream_addr = 0;
    BLT_Result                result;

    ATX_LOG_FINE("create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR ||
        constructor->name == NULL ||
        ATX_StringLength(constructor->name) < 15) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* check that we support pointers as integers */
    if (sizeof(void*) > sizeof(stream_addr)) {
        return ATX_ERROR_NOT_SUPPORTED;
    }

    /* parse the name */
    result = ATX_ParseInteger64(constructor->name+15, &stream_addr, ATX_FALSE);
    if (ATX_FAILED(result)) return result;

    /* allocate memory for the object */
    input = (CallbackInput*)ATX_AllocateZeroMemory(sizeof(CallbackInput));
    if (input == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(input, BLT_BaseMediaNode), module, core);
    
    /* keep a reference to the stream */
    input->stream = (ATX_InputStream*)(ATX_IntPtr)stream_addr;
    ATX_REFERENCE_OBJECT(input->stream);

    /* remember media type */
    BLT_MediaType_Clone(constructor->spec.output.media_type, 
                        &input->media_type);

    /* construct reference */
    ATX_SET_INTERFACE_EX(input, CallbackInput, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(input, CallbackInput, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE   (input, CallbackInput, BLT_MediaPort);
    ATX_SET_INTERFACE   (input, CallbackInput, BLT_InputStreamProvider);
    *object = &ATX_BASE_EX(input, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    CallbackInput_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
CallbackInput_Destroy(CallbackInput* self)
{
    ATX_LOG_FINE("CallbackInput::Destroy");

    /* release the byte stream */
    ATX_RELEASE_OBJECT(self->stream);
    
    /* free the media type extensions */
    BLT_MediaType_Free(self->media_type);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CallbackInput_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
CallbackInput_GetPortByName(BLT_MediaNode*  _self,
                            BLT_CString     name,
                            BLT_MediaPort** port)
{
    CallbackInput* self = ATX_SELF_EX(CallbackInput, BLT_BaseMediaNode, BLT_MediaNode);
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
|    CallbackInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
CallbackInput_QueryMediaType(BLT_MediaPort*        _self,
                             BLT_Ordinal           index,
                             const BLT_MediaType** media_type)
{
    CallbackInput* self = ATX_SELF(CallbackInput, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = self->media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   CallbackInput_GetStream
+---------------------------------------------------------------------*/
BLT_METHOD
CallbackInput_GetStream(BLT_InputStreamProvider* _self,
                        ATX_InputStream**        stream)
{
    CallbackInput* self = ATX_SELF(CallbackInput, BLT_InputStreamProvider);

    /* return our stream object */
    *stream = self->stream;
    ATX_REFERENCE_OBJECT(*stream);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    CallbackInput_Activate
+---------------------------------------------------------------------*/
BLT_METHOD
CallbackInput_Activate(BLT_MediaNode* _self, BLT_Stream* stream)
{
    CallbackInput* self = ATX_SELF_EX(CallbackInput, BLT_BaseMediaNode, BLT_MediaNode);

    /* update the stream info */
    {
        BLT_StreamInfo info;
        ATX_LargeSize  size;
        BLT_Result     result;

        result = ATX_InputStream_GetSize(self->stream, &size);
        if (BLT_SUCCEEDED(result)) {
            info.mask = BLT_STREAM_INFO_MASK_SIZE;
            info.size = size;
            BLT_Stream_SetInfo(stream, &info);
        }
    }
    
    /* keep the stream as our context */
    ATX_BASE(self, BLT_BaseMediaNode).context = stream;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(CallbackInput)
    ATX_GET_INTERFACE_ACCEPT_EX(CallbackInput, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(CallbackInput, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT   (CallbackInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT   (CallbackInput, BLT_InputStreamProvider)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(CallbackInput, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    CallbackInput_GetPortByName,
    CallbackInput_Activate,
    BLT_BaseMediaNode_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    BLT_BaseMediaNode_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(CallbackInput, 
                                         "output", 
                                         STREAM_PULL, 
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(CallbackInput, BLT_MediaPort)
    CallbackInput_GetName,
    CallbackInput_GetProtocol,
    CallbackInput_GetDirection,
    CallbackInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_InputStreamProvider interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(CallbackInput, BLT_InputStreamProvider)
    CallbackInput_GetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(CallbackInput, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   CallbackInputModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
CallbackInputModule_Probe(BLT_Module*              self, 
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

            /* the input protocol should be NONE, and the output */
            /* protocol should be STREAM_PULL                    */
            if ((constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_NONE) ||
                (constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL)) {
                return BLT_FAILURE;
            }

            /* check the name */
            if (!ATX_StringsEqualN(constructor->name, "callback-input:", 15)) {
                return BLT_FAILURE;
            }
                
            /* this is an exact match for us */
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(CallbackInputModule)
    ATX_GET_INTERFACE_ACCEPT_EX(CallbackInputModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(CallbackInputModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(CallbackInputModule, CallbackInput)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(CallbackInputModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    CallbackInputModule_CreateInstance,
    CallbackInputModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define CallbackInputModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(CallbackInputModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_Result 
BLT_CallbackInputModule_GetModuleObject(BLT_Module** object)
{
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    return BLT_BaseModule_Create("Callback Input", NULL, 0,
                                 &CallbackInputModule_BLT_ModuleInterface,
                                 &CallbackInputModule_ATX_ReferenceableInterface,
                                 object);
}
