/*****************************************************************
|
|   BlueTune - File Input Module
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
#include "BltFileInput.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltModule.h"
#include "BltByteStreamProvider.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.inputs.file")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);
} FileInputModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(ATX_InputStream);
    ATX_IMPLEMENTS(ATX_Referenceable);

    /* members */
    ATX_Cardinal     reference_count;
    ATX_File*        file;
    ATX_InputStream* stream;
    ATX_Position     detached_position;
} FileInputStream;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);
    
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamProvider);

    /* members */
    FileInputStream* file_stream;
    BLT_MediaType*   media_type;
} FileInput;


/*----------------------------------------------------------------------
|    forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(FileInputModule, BLT_Module)

ATX_DECLARE_INTERFACE_MAP(FileInputStream, ATX_InputStream)
ATX_DECLARE_INTERFACE_MAP(FileInputStream, ATX_Referenceable)

ATX_DECLARE_INTERFACE_MAP(FileInput, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(FileInput, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(FileInput, BLT_MediaPort)
ATX_DECLARE_INTERFACE_MAP(FileInput, BLT_InputStreamProvider)
static BLT_Result FileInput_Destroy(FileInput* self);

/*----------------------------------------------------------------------
|    FileInput_DecideMediaType
+---------------------------------------------------------------------*/
static BLT_Result
FileInput_DecideMediaType(FileInput* self, BLT_CString name)
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
|    FileInputStream_Create
+---------------------------------------------------------------------*/
static ATX_Result
FileInputStream_Create(const char* filename, FileInputStream** object)
{
    ATX_Result       result;
    FileInputStream* self;
    
    /* default value */
    *object = NULL;
    
    /* allocate the object */
    self = (FileInputStream*)ATX_AllocateZeroMemory(sizeof(FileInputStream));
    if (self == NULL) return ATX_ERROR_OUT_OF_MEMORY;
    self->reference_count = 1;
    
    /* create the file object */
    result = ATX_File_Create(filename, &self->file);
    if (ATX_FAILED(result)) {
        ATX_LOG_WARNING_1("cannot create file (%d)", result);
        goto end;
    }
    
    /* open and close the file to make sure it is readable */
    result = ATX_File_Open(self->file, ATX_FILE_OPEN_MODE_READ);
    if (ATX_FAILED(result)) {
        ATX_LOG_WARNING_1("cannot open file (%d)", result);
        return result;
    }
    result = ATX_File_Close(self->file);
    if (ATX_FAILED(result)) {
        ATX_LOG_WARNING_1("cannot close file (%d)", result);
        return result;
    }

    ATX_SET_INTERFACE(self, FileInputStream, ATX_InputStream);
    ATX_SET_INTERFACE(self, FileInputStream, ATX_Referenceable);
    *object = self;

end:
    if (ATX_FAILED(result)) {
        ATX_FreeMemory(self);
        *object = NULL;
    }
    return result;
}

/*----------------------------------------------------------------------
|    FileInputStream_Destroy
+---------------------------------------------------------------------*/
static void
FileInputStream_Destroy(FileInputStream* self)
{
    if (self->stream) {
        ATX_RELEASE_OBJECT(self->stream);
        ATX_File_Close(self->file);
    }
    if (self->file) {
        ATX_DESTROY_OBJECT(self->file);
    }
    ATX_FreeMemory(self);
}

/*----------------------------------------------------------------------
|    FileInputStream_Detach
+---------------------------------------------------------------------*/
static BLT_Result
FileInputStream_Detach(FileInputStream* self)
{
    /* remember where we detached */
    if (self->stream) {
        ATX_Result result = ATX_InputStream_Tell(self->stream, &self->detached_position);
        if (ATX_FAILED(result)) self->detached_position = 0;
        ATX_RELEASE_OBJECT(self->stream);
        ATX_LOG_FINE_1("detaching at position %d", (int)self->detached_position);
    }
    ATX_File_Close(self->file);
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    FileInputStream_Attach
+---------------------------------------------------------------------*/
static BLT_Result
FileInputStream_Attach(FileInputStream* self)
{
    ATX_Result result;
    
    /* do nothing if we're already attached */
    if (self->stream) return BLT_SUCCESS;
    
    /* re-open the source and get the stream */
    ATX_LOG_FINE("attaching to file");
    result = ATX_File_Open(self->file, ATX_FILE_OPEN_MODE_READ);
    if (ATX_FAILED(result)) {
        ATX_LOG_WARNING_1("failed to reopen file (%d)", result);
        return result;
    }
    result = ATX_File_GetInputStream(self->file, &self->stream);
    if (ATX_FAILED(result)) return result;
    
    /* restore the position to where we last detached */
    if (self->detached_position) {
        result = ATX_InputStream_Seek(self->stream, self->detached_position);
        if (ATX_FAILED(result)) return result;
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    FileInputStream_Read
+---------------------------------------------------------------------*/
ATX_METHOD
FileInputStream_Read(ATX_InputStream* _self,
                     ATX_Any          buffer, 
                     ATX_Size         bytes_to_read, 
                     ATX_Size*        bytes_read)
{
    FileInputStream* self = ATX_SELF(FileInputStream, ATX_InputStream);
    if (!self->stream) {
        BLT_Result result = FileInputStream_Attach(self);
        if (BLT_FAILED(result)) return result;
    }

    return ATX_InputStream_Read(self->stream, buffer, bytes_to_read, bytes_read);
}

/*----------------------------------------------------------------------
|    FileInputStream_Seek
+---------------------------------------------------------------------*/
ATX_METHOD
FileInputStream_Seek(ATX_InputStream* _self, 
                     ATX_Position     where)
{
    FileInputStream* self = ATX_SELF(FileInputStream, ATX_InputStream);
    if (!self->stream) {
        BLT_Result result = FileInputStream_Attach(self);
        if (BLT_FAILED(result)) return result;
    }

    return ATX_InputStream_Seek(self->stream, where);
}

/*----------------------------------------------------------------------
|    FileInputStream_Tell
+---------------------------------------------------------------------*/
ATX_METHOD
FileInputStream_Tell(ATX_InputStream* _self, 
                     ATX_Position*    where)
{
    FileInputStream* self = ATX_SELF(FileInputStream, ATX_InputStream);
    if (!self->stream) {
        BLT_Result result = FileInputStream_Attach(self);
        if (BLT_FAILED(result)) return result;
    }

    return ATX_InputStream_Tell(self->stream, where);
}

/*----------------------------------------------------------------------
|    FileInputStream_GetSize
+---------------------------------------------------------------------*/
ATX_METHOD
FileInputStream_GetSize(ATX_InputStream* _self, 
                        ATX_LargeSize*   size)
{
    FileInputStream* self = ATX_SELF(FileInputStream, ATX_InputStream);
    if (!self->stream) {
        BLT_Result result = FileInputStream_Attach(self);
        if (BLT_FAILED(result)) return result;
    }

    return ATX_InputStream_GetSize(self->stream, size);
}

/*----------------------------------------------------------------------
|    FileInputStream_GetAvailable
+---------------------------------------------------------------------*/
ATX_METHOD
FileInputStream_GetAvailable(ATX_InputStream* _self, 
                             ATX_LargeSize*   available)
{
    FileInputStream* self = ATX_SELF(FileInputStream, ATX_InputStream);
    if (!self->stream) {
        BLT_Result result = FileInputStream_Attach(self);
        if (BLT_FAILED(result)) return result;
    }

    return ATX_InputStream_GetAvailable(self->stream, available);
}

/*----------------------------------------------------------------------
|   FileInputStream_GetInterface
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(FileInputStream)
    ATX_GET_INTERFACE_ACCEPT(FileInputStream, ATX_InputStream)
    ATX_GET_INTERFACE_ACCEPT(FileInputStream, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   ATX_InputStream interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(FileInputStream, ATX_InputStream)
    FileInputStream_Read,
    FileInputStream_Seek,
    FileInputStream_Tell,
    FileInputStream_GetSize,
    FileInputStream_GetAvailable
};

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE(FileInputStream, reference_count)

/*----------------------------------------------------------------------
|    FileInput_Create
+---------------------------------------------------------------------*/
static BLT_Result
FileInput_Create(BLT_Module*              module,
                 BLT_Core*                core, 
                 BLT_ModuleParametersType parameters_type,
                 BLT_AnyConst             parameters, 
                 BLT_MediaNode**          object)
{
    FileInput*                input;
    BLT_MediaNodeConstructor* constructor = 
        (BLT_MediaNodeConstructor*)parameters;
    BLT_Result                result;

    ATX_LOG_FINE("FileInput::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR ||
        constructor->name == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    input = (FileInput*)ATX_AllocateZeroMemory(sizeof(FileInput));
    if (input == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(input, BLT_BaseMediaNode), module, core);
    
    /* strip the "file:" prefix if it is present */
    if (ATX_StringsEqualN(constructor->name, "file:", 5)) {
        constructor->name += 5;
    }

    /* create the file input stream */
    result = FileInputStream_Create(constructor->name, &input->file_stream);
    if (ATX_FAILED(result)) {
        input->file_stream = NULL;
        goto failure;
    }

    /* figure out the media type */
    if (constructor->spec.output.media_type->id == BLT_MEDIA_TYPE_ID_UNKNOWN ||
        constructor->spec.output.media_type->id == BLT_MEDIA_TYPE_ID_AUDIO   ||
        constructor->spec.output.media_type->id == BLT_MEDIA_TYPE_ID_VIDEO) {
        /* unknown type, try to figure it out from the file extension */
        BLT_MediaType_Clone(&BLT_MediaType_Unknown, &input->media_type);
        FileInput_DecideMediaType(input, constructor->name);
    } else {
        /* use the media type from the output spec */
        BLT_MediaType_Clone(constructor->spec.output.media_type, 
                            &input->media_type);
    }

    /* construct reference */
    ATX_SET_INTERFACE_EX(input, FileInput, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(input, FileInput, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE   (input, FileInput, BLT_MediaPort);
    ATX_SET_INTERFACE   (input, FileInput, BLT_InputStreamProvider);
    *object = &ATX_BASE_EX(input, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;

failure:
    FileInput_Destroy(input);
    object = NULL;
    return result;
}

/*----------------------------------------------------------------------
|    FileInput_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
FileInput_Destroy(FileInput* self)
{
    ATX_LOG_FINE("FileInput::Destroy");

    /* release the file input stream */
    if (self->file_stream) FileInputStream_Release(&ATX_BASE(self->file_stream, ATX_Referenceable));

    /* free the media type extensions */
    BLT_MediaType_Free(self->media_type);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   FileInput_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
FileInput_GetPortByName(BLT_MediaNode*  _self,
                        BLT_CString     name,
                        BLT_MediaPort** port)
{
    FileInput* self = ATX_SELF_EX(FileInput, BLT_BaseMediaNode, BLT_MediaNode);
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
|    FileInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
FileInput_QueryMediaType(BLT_MediaPort*        _self,
                         BLT_Ordinal           index,
                         const BLT_MediaType** media_type)
{
    FileInput* self = ATX_SELF(FileInput, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = self->media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   FileInput_GetStream
+---------------------------------------------------------------------*/
BLT_METHOD
FileInput_GetStream(BLT_InputStreamProvider* _self,
                    ATX_InputStream**        stream)
{
    FileInput* self = ATX_SELF(FileInput, BLT_InputStreamProvider);

    /* return our stream object */
    *stream = &ATX_BASE(self->file_stream, ATX_InputStream);
    ATX_REFERENCE_OBJECT(*stream);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    FileInput_Activate
+---------------------------------------------------------------------*/
BLT_METHOD
FileInput_Activate(BLT_MediaNode* _self, BLT_Stream* stream)
{
    FileInput* self = ATX_SELF_EX(FileInput, BLT_BaseMediaNode, BLT_MediaNode);
    
    /* update the stream info */
    {
        BLT_StreamInfo info;
        ATX_LargeSize  file_size;
        BLT_Result     result;

        result = ATX_File_GetSize(self->file_stream->file, &file_size);
        if (BLT_SUCCEEDED(result)) {
            info.mask = BLT_STREAM_INFO_MASK_SIZE;
            info.size = file_size;
            BLT_Stream_SetInfo(stream, &info);
        }
    }
    
    /* keep the stream as our context */
    ATX_BASE(self, BLT_BaseMediaNode).context = stream;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   FileInput_Start
+---------------------------------------------------------------------*/
BLT_METHOD
FileInput_Start(BLT_MediaNode* _self)
{
    FileInput* self = ATX_SELF_EX(FileInput, BLT_BaseMediaNode, BLT_MediaNode);
    return FileInputStream_Attach(self->file_stream);
}

/*----------------------------------------------------------------------
|   FileInput_Stop
+---------------------------------------------------------------------*/
BLT_METHOD
FileInput_Stop(BLT_MediaNode* _self)
{
    FileInput* self = ATX_SELF_EX(FileInput, BLT_BaseMediaNode, BLT_MediaNode);
    FileInputStream_Detach(self->file_stream);
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(FileInput)
    ATX_GET_INTERFACE_ACCEPT_EX(FileInput, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(FileInput, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT   (FileInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT   (FileInput, BLT_InputStreamProvider)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(FileInput, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    FileInput_GetPortByName,
    FileInput_Activate,
    BLT_BaseMediaNode_Deactivate,
    FileInput_Start,
    FileInput_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    BLT_BaseMediaNode_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(FileInput, 
                                         "output", 
                                         STREAM_PULL, 
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(FileInput, BLT_MediaPort)
    FileInput_GetName,
    FileInput_GetProtocol,
    FileInput_GetDirection,
    FileInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_InputStreamProvider interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(FileInput, BLT_InputStreamProvider)
    FileInput_GetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(FileInput, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   FileInputModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
FileInputModule_Probe(BLT_Module*              self, 
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
            if (ATX_StringsEqualN(constructor->name, "file:", 5)) {
                /* this is an exact match for us */
                *match = BLT_MODULE_PROBE_MATCH_EXACT;
            } else if (constructor->spec.input.protocol ==
                       BLT_MEDIA_PORT_PROTOCOL_NONE) {
                /* default match level */
                *match = BLT_MODULE_PROBE_MATCH_DEFAULT;
            } else {
                return BLT_FAILURE;
            }

            ATX_LOG_FINE_1("FileInputModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(FileInputModule)
    ATX_GET_INTERFACE_ACCEPT_EX(FileInputModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(FileInputModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(FileInputModule, FileInput)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(FileInputModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    FileInputModule_CreateInstance,
    FileInputModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define FileInputModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(FileInputModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(FileInputModule,
                                         "File Input",
                                         "com.axiosys.input.file",
                                         "1.0.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
