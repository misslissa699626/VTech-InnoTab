/*****************************************************************
|
|   BlueTune - Network Input Module
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
#include "BltNetworkInput.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltModule.h"
#include "BltByteStreamProvider.h"
#include "BltTcpNetworkStream.h"
#include "BltHttpNetworkStream.h"
#include "BltNetworkInputSource.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.inputs.network")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);
} NetworkInputModule;

typedef struct {
    /* interfaces */
    ATX_EXTENDS(BLT_BaseMediaNode);
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamProvider);

    /* members */
    BLT_NetworkInputSource* source;
    ATX_InputStream*        stream;
    ATX_Flags               flags;
    BLT_MediaType*          media_type;
} NetworkInput;

/*----------------------------------------------------------------------
|    forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(NetworkInputModule, BLT_Module)

ATX_DECLARE_INTERFACE_MAP(NetworkInput, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(NetworkInput, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(NetworkInput, BLT_MediaPort)
ATX_DECLARE_INTERFACE_MAP(NetworkInput, BLT_InputStreamProvider)

/*----------------------------------------------------------------------
|    NetworkInput_DecideMediaType
+---------------------------------------------------------------------*/
static BLT_Result
NetworkInput_DecideMediaType(NetworkInput* self, BLT_CString name)
{
    BLT_Registry* registry;
    BLT_CString   extension;
    BLT_Result    result;

    /* compute file extension */
    extension = NULL;
    while (*name) {
        if (*name == '.') {
            extension = name;
        }
        name++;
    }
    if (extension == NULL) return BLT_SUCCESS;

    /* get the registry */
    result = BLT_Core_GetRegistry(ATX_BASE(self, BLT_BaseMediaNode).core, &registry);
    if (BLT_FAILED(result)) return result;

    /* query the registry */
    return BLT_Registry_GetMediaTypeIdForExtension(registry, 
                                                   extension, 
                                                   &self->media_type->id);
}

/*----------------------------------------------------------------------
|    NetworkInput_Create
+---------------------------------------------------------------------*/
static BLT_Result
NetworkInput_Create(BLT_Module*              module,
                    BLT_Core*                core, 
                    BLT_ModuleParametersType parameters_type,
                    BLT_CString              parameters, 
                    BLT_MediaNode**          object)
{
    NetworkInput*             input;
    BLT_MediaNodeConstructor* constructor = (BLT_MediaNodeConstructor*)parameters;
    BLT_Result                result;

    ATX_LOG_FINE("NetworkInput::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR ||
        constructor->name == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    input = (NetworkInput*)ATX_AllocateZeroMemory(sizeof(NetworkInput));
    if (input == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(input, BLT_BaseMediaNode), module, core);
    
    /* create the network stream */
    if (ATX_StringsEqualN(constructor->name, "tcp://", 6)) {
        /* create a TCP byte stream */
        result = BLT_TcpNetworkStream_Create(constructor->name+6, &input->stream);
    } else if (ATX_StringsEqualN(constructor->name, "http://", 7)) {
        /* create an HTTP byte stream */
        result = BLT_HttpNetworkStream_Create(constructor->name, 
                                              core, 
                                              &input->stream,
                                              &input->source, 
                                              &input->media_type);
    } else {
        result = BLT_ERROR_INVALID_PARAMETERS;
    }

    if (ATX_FAILED(result)) {
        input->stream = NULL;
        ATX_FreeMemory(input);
        return result;
    }

    /* figure out the media type */
    if (input->media_type == NULL) {
        if (constructor->spec.output.media_type->id == BLT_MEDIA_TYPE_ID_UNKNOWN ||
            constructor->spec.output.media_type->id == BLT_MEDIA_TYPE_ID_AUDIO   ||
            constructor->spec.output.media_type->id == BLT_MEDIA_TYPE_ID_VIDEO) {
            /* unknown type, try to figure it out from the name extension */
            BLT_MediaType_Clone(&BLT_MediaType_Unknown, &input->media_type);
            NetworkInput_DecideMediaType(input, constructor->name);
        } else {
            /* use the media type from the output spec */
            BLT_MediaType_Clone(constructor->spec.output.media_type, 
                                &input->media_type);
        }
    }

    /* construct reference */
    ATX_SET_INTERFACE_EX(input, NetworkInput, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(input, NetworkInput, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE   (input, NetworkInput, BLT_MediaPort);
    ATX_SET_INTERFACE   (input, NetworkInput, BLT_InputStreamProvider);
    *object = &ATX_BASE_EX(input, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    NetworkInput_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
NetworkInput_Destroy(NetworkInput* self)
{
    ATX_LOG_FINE("NetworkInput::Destroy");

    /* release the input stream */
    ATX_RELEASE_OBJECT(self->stream);
    
    /* release the input source */
    ATX_RELEASE_OBJECT(self->source);

    /* free the media type extensions */
    BLT_MediaType_Free(self->media_type);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NetworkInput_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
NetworkInput_GetPortByName(BLT_MediaNode*  _self,
                           BLT_CString     name,
                           BLT_MediaPort** port)
{
    NetworkInput* self = ATX_SELF_EX(NetworkInput, BLT_BaseMediaNode, BLT_MediaNode);
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
|    NetworkInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
NetworkInput_QueryMediaType(BLT_MediaPort*        _self,
                            BLT_Ordinal           index,
                            const BLT_MediaType** media_type)
{
    NetworkInput* self = ATX_SELF(NetworkInput, BLT_MediaPort);

    if (index == 0) {
        *media_type = self->media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   NetworkInput_GetStream
+---------------------------------------------------------------------*/
BLT_METHOD
NetworkInput_GetStream(BLT_InputStreamProvider* _self,
                       ATX_InputStream**        stream)
{
    NetworkInput* self = ATX_SELF(NetworkInput, BLT_InputStreamProvider);

    /* return our stream object */
    *stream = self->stream;
    ATX_REFERENCE_OBJECT(*stream);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    NetworkInput_Activate
+---------------------------------------------------------------------*/
BLT_METHOD
NetworkInput_Activate(BLT_MediaNode* _self, BLT_Stream* stream)
{
    NetworkInput* self = ATX_SELF_EX(NetworkInput, BLT_BaseMediaNode, BLT_MediaNode);

    /* update the stream info */
    {
        BLT_StreamInfo info;
        BLT_Result     result;
        ATX_LargeSize  size;

        result = ATX_InputStream_GetSize(self->stream, &size);
        if (BLT_SUCCEEDED(result)) {
            info.size = size;
            info.mask = BLT_STREAM_INFO_MASK_SIZE;
            BLT_Stream_SetInfo(stream, &info);
        } else {
            info.size = 0;
        }
    }
    
    /* keep the stream as our context */
    ATX_BASE(self, BLT_BaseMediaNode).context = stream;

    /* notify the source */
    if (self->source) {
        return BLT_NetworkInputSource_Attach(self->source, stream);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    NetworkInput_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
NetworkInput_Deactivate(BLT_MediaNode* _self)
{
    NetworkInput* self = ATX_SELF_EX(NetworkInput, BLT_BaseMediaNode, BLT_MediaNode);

    /* notify the source */
    if (self->source) {
        return BLT_NetworkInputSource_Detach(self->source);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(NetworkInput)
    ATX_GET_INTERFACE_ACCEPT_EX(NetworkInput, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(NetworkInput, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT   (NetworkInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT   (NetworkInput, BLT_InputStreamProvider)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(NetworkInput, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    NetworkInput_GetPortByName,
    NetworkInput_Activate,
    NetworkInput_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    BLT_BaseMediaNode_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(NetworkInput, 
                                         "output", 
                                         STREAM_PULL, 
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(NetworkInput, BLT_MediaPort)
    NetworkInput_GetName,
    NetworkInput_GetProtocol,
    NetworkInput_GetDirection,
    NetworkInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_InputStreamProvider interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(NetworkInput, BLT_InputStreamProvider)
    NetworkInput_GetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(NetworkInput, 
                                         BLT_BaseMediaNode, 
                                         reference_count)


/*----------------------------------------------------------------------
|   NetworkInputModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
NetworkInputModule_Probe(BLT_Module*              self, 
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

            /* we need a file name */
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
            if (ATX_StringsEqualN(constructor->name, "tcp://", 6) ||
                ATX_StringsEqualN(constructor->name, "http://", 7)) {
                /* this is an exact match for us */
                *match = BLT_MODULE_PROBE_MATCH_EXACT;
            } else if (constructor->spec.input.protocol ==
                       BLT_MEDIA_PORT_PROTOCOL_NONE) {
                /* default match level */
                *match = BLT_MODULE_PROBE_MATCH_DEFAULT;
            } else {
                return BLT_FAILURE;
            }

            ATX_LOG_FINE_1("NetworkInputModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(NetworkInputModule)
    ATX_GET_INTERFACE_ACCEPT_EX(NetworkInputModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(NetworkInputModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(NetworkInputModule, NetworkInput)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(NetworkInputModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    NetworkInputModule_CreateInstance,
    NetworkInputModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define NetworkInputModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(NetworkInputModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(NetworkInputModule,
                                         "Network Input",
                                         "com.axiosys.input.network",
                                         "1.0.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
