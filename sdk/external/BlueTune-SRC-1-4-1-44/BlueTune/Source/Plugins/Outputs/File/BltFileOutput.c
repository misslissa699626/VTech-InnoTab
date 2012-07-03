/*****************************************************************
|
|   File Output Module
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
#include "BltFileOutput.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltByteStreamProvider.h"
#include "BltPcm.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.outputs.file")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);
} FileOutputModule;

typedef struct {
    /* base class */
    ATX_EXTENDS   (BLT_BaseMediaNode);

    /* interfaces */
    ATX_IMPLEMENTS(BLT_OutputStreamProvider);
    ATX_IMPLEMENTS(BLT_OutputNode);
    ATX_IMPLEMENTS(BLT_MediaPort);

    /* members */
    ATX_File*         file;
    ATX_OutputStream* stream;
    BLT_MediaType*    media_type;
} FileOutput;

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(FileOutputModule, BLT_Module)

ATX_DECLARE_INTERFACE_MAP(FileOutput, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(FileOutput, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(FileOutput, BLT_OutputNode)
ATX_DECLARE_INTERFACE_MAP(FileOutput, BLT_MediaPort)
ATX_DECLARE_INTERFACE_MAP(FileOutput, BLT_OutputStreamProvider)
static BLT_Result FileOutput_Destroy(FileOutput* self);

/*----------------------------------------------------------------------
|    FileOutput_DecideMediaType
+---------------------------------------------------------------------*/
static BLT_Result
FileOutput_DecideMediaType(FileOutput* self, BLT_CString name)
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
|   FileOutput_GetStream
+---------------------------------------------------------------------*/
BLT_METHOD
FileOutput_GetStream(BLT_OutputStreamProvider* _self,
                     ATX_OutputStream**        stream,
                     const BLT_MediaType*      media_type)
{
    FileOutput* self = ATX_SELF(FileOutput, BLT_OutputStreamProvider);

    *stream = self->stream;
    ATX_REFERENCE_OBJECT(*stream);

    /* we're providing the stream, but we *receive* the type */
    if (media_type) {
        if (self->media_type->id == BLT_MEDIA_TYPE_ID_UNKNOWN) {
            BLT_MediaType_Free(self->media_type);
            BLT_MediaType_Clone(media_type, &self->media_type);
        } else if (self->media_type->id != media_type->id) {
            return BLT_ERROR_INVALID_MEDIA_TYPE;
        }
    }
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    FileOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
FileOutput_QueryMediaType(BLT_MediaPort*        _self,
                          BLT_Ordinal           index,
                          const BLT_MediaType** media_type)
{
    FileOutput* self = ATX_SELF(FileOutput, BLT_MediaPort);
    if (index == 0) {
        *media_type = self->media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|    FileOutput_Create
+---------------------------------------------------------------------*/
static BLT_Result
FileOutput_Create(BLT_Module*              module,
                  BLT_Core*                core, 
                  BLT_ModuleParametersType parameters_type,
                  BLT_CString              parameters, 
                  BLT_MediaNode**          object)
{
    FileOutput*               self;
    BLT_MediaNodeConstructor* constructor = (BLT_MediaNodeConstructor*)parameters;
    BLT_Result                result;

    ATX_LOG_FINE("FileOutput::Create");

    /* check parameters */
    *object = NULL;
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* check the name */
    if (constructor->name == NULL || ATX_StringLength(constructor->name) < 4) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }
        
    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(FileOutput));
    if (self == NULL) {
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* figure out the media type */
    if (constructor->spec.input.media_type->id == BLT_MEDIA_TYPE_ID_UNKNOWN) {
        /* unknown type, try to figure it out from the file extension */
        BLT_MediaType_Clone(&BLT_MediaType_Unknown, &self->media_type);
        result = FileOutput_DecideMediaType(self, constructor->name);
        if (BLT_FAILED(result)) {
            /* if the type is not found, assume audio/pcm */
            BLT_PcmMediaType pcm_type;
            BLT_PcmMediaType_Init(&pcm_type);
            BLT_MediaType_Clone((BLT_MediaType*)&pcm_type, &self->media_type);
        }
    } else {
        /* use the media type from the input spec */
        BLT_MediaType_Clone(constructor->spec.input.media_type,
                            &self->media_type);
    }

    /* create the output file object */
    result = ATX_File_Create(constructor->name+5, &self->file);
    if (BLT_FAILED(result)) {
        self->file = NULL;
        goto failure;
    }

    /* open the output file */
    result = ATX_File_Open(self->file,
                           ATX_FILE_OPEN_MODE_WRITE  |
                           ATX_FILE_OPEN_MODE_CREATE |
                           ATX_FILE_OPEN_MODE_TRUNCATE);
    if (ATX_FAILED(result)) goto failure;

    /* get the output stream */
    result = ATX_File_GetOutputStream(self->file, &self->stream);
    if (BLT_FAILED(result)) {
        self->stream = NULL;
        goto failure;
    }

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, FileOutput, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, FileOutput, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(self, FileOutput, BLT_OutputStreamProvider);
    ATX_SET_INTERFACE(self, FileOutput, BLT_OutputNode);
    ATX_SET_INTERFACE(self, FileOutput, BLT_MediaPort);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;

 failure:
    FileOutput_Destroy(self);
    *object = NULL;
    return result;
}

/*----------------------------------------------------------------------
|    FileOutput_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
FileOutput_Destroy(FileOutput* self)
{
    ATX_LOG_FINE("FileOutput::Destroy");

    /* release the stream */
    ATX_RELEASE_OBJECT(self->stream);

    /* destroy the file */
    ATX_DESTROY_OBJECT(self->file);

    /* free the media type extensions */
    BLT_MediaType_Free(self->media_type);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   FileOutput_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
FileOutput_GetPortByName(BLT_MediaNode*  _self,
                         BLT_CString     name,
                         BLT_MediaPort** port)
{
    FileOutput* self = ATX_SELF_EX(FileOutput, BLT_BaseMediaNode, BLT_MediaNode);

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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(FileOutput)
    ATX_GET_INTERFACE_ACCEPT_EX(FileOutput, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(FileOutput, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT(FileOutput, BLT_OutputNode)
    ATX_GET_INTERFACE_ACCEPT(FileOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(FileOutput, BLT_OutputStreamProvider)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(FileOutput, 
                                         "input", 
                                         STREAM_PUSH,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(FileOutput, BLT_MediaPort)
    FileOutput_GetName,
    FileOutput_GetProtocol,
    FileOutput_GetDirection,
    FileOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_OutputStreamProvider interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(FileOutput, BLT_OutputStreamProvider)
    FileOutput_GetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(FileOutput, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    FileOutput_GetPortByName,
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
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(FileOutput, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   FileOutputModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
FileOutputModule_Probe(BLT_Module*              self, 
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

            /* the input protocol should be STREAM_PUSH and the */
            /* output protocol should be NONE                   */
            if ((constructor->spec.input.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_STREAM_PUSH) ||
                (constructor->spec.output.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_NONE)) {
                return BLT_FAILURE;
            }

            /* we need a name */
            if (constructor->name == NULL) {
                return BLT_FAILURE;
            }

            /* the name needs to be file:<filename> */
            if (!ATX_StringsEqualN(constructor->name, "file:", 5)) {
                return BLT_FAILURE;
            }

            /* always an exact match, since we only respond to our name */
            *match = BLT_MODULE_PROBE_MATCH_EXACT;

            ATX_LOG_FINE_1("FileOutputModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(FileOutputModule)
    ATX_GET_INTERFACE_ACCEPT_EX(FileOutputModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(FileOutputModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(FileOutputModule, FileOutput)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(FileOutputModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    FileOutputModule_CreateInstance,
    FileOutputModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define FileOutputModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(FileOutputModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(FileOutputModule,
                                         "File Output",
                                         "com.axiosys.output.file",
                                         "1.2.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
