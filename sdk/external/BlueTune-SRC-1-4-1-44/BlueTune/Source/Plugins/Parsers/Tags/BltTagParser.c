/*****************************************************************
|
|   Tag Parser Module
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
#include "BltTagParser.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltByteStreamProvider.h"
#include "BltByteStreamUser.h"
#include "BltId3Parser.h"
#include "BltApeParser.h"
#include "BltEventListener.h"
#include "BltStream.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.parsers.tag")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_UInt32 mpeg_audio_type_id;
} TagParserModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamUser);

    /* members */
    BLT_MediaType    media_type;
    ATX_InputStream* stream;
} TagParserInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamProvider);
} TagParserOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    TagParserInput  input;
    TagParserOutput output;
} TagParser;

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define BLT_TAG_PARSER_MEDIA_TYPE_FLAGS_PARSED 1

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(TagParserModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(TagParser, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(TagParser, ATX_Referenceable)

/*----------------------------------------------------------------------
|   TagParserInput_SetStream
+---------------------------------------------------------------------*/
BLT_METHOD
TagParserInput_SetStream(BLT_InputStreamUser* _self,
                         ATX_InputStream*     stream,
                         const BLT_MediaType* media_type)
{
    TagParser*      self = ATX_SELF_M(input, TagParser, BLT_InputStreamUser);
    BLT_Size        id3_header_size  = 0;
    BLT_Size        id3_trailer_size = 0;
    /*BLT_Size       ape_trailer_size = 0;*/
    BLT_Size        header_size      = 0;
    BLT_Size        trailer_size     = 0;
    ATX_Position    stream_start;
    BLT_LargeSize   stream_size;
    ATX_Properties* stream_properties;
    BLT_Result      result;

    /* check media type */
    if (media_type == NULL || media_type->id != self->input.media_type.id) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* get a reference to the stream properties */
    result = BLT_Stream_GetProperties(ATX_BASE(self, BLT_BaseMediaNode).context, &stream_properties);
    if (BLT_FAILED(result)) return result;

    /* if we had a stream, release it */
    ATX_RELEASE_OBJECT(self->input.stream);

    /* remember the start of the stream */
    result = ATX_InputStream_Tell(stream, &stream_start);
    if (BLT_FAILED(result)) return result;

    /* get the stream size */
    result = ATX_InputStream_GetSize(stream, &stream_size);
    if (BLT_FAILED(result) || stream_size < stream_start) {
        stream_size = 0;
    } else {
        stream_size -= stream_start;
    }

    /* parse the ID3 header and trailer */
    result = BLT_Id3Parser_ParseStream(stream, 
                                       stream_start,
                                       stream_size,
                                       &id3_header_size,
                                       &id3_trailer_size,
                                       stream_properties);
    if (BLT_SUCCEEDED(result)) {
        header_size = id3_header_size;
        trailer_size = id3_trailer_size;
        if (stream_size >= id3_header_size+id3_trailer_size) {
            stream_size -= id3_header_size+id3_trailer_size;
        }
    }

    /* parse APE tags */
#if 0 /* disable for now */
    result = BLT_ApeParser_ParseStream(stream,
                                       stream_start + header_size,
                                       stream_size,
                                       &ape_trailer_size,
                                       &stream_properties);
    if (BLT_SUCCEEDED(result)) {
        trailer_size += ape_trailer_size;
        if (stream_size >= ape_trailer_size) {
            stream_size -= ape_trailer_size;
        }
    } 
#endif

    if (header_size != 0 || trailer_size != 0) {
        /* create a sub stream without the header and the trailer */
        ATX_LOG_FINER_3("TagParserInput_SetStream: substream %ld [%ld - %ld]",
                         stream_size, header_size, trailer_size);
        result = ATX_SubInputStream_Create(stream, 
                                           header_size, 
                                           stream_size,
                                           NULL,
                                           &self->input.stream);        
        if (ATX_FAILED(result)) return result;

        /* update the stream info */
        {
            BLT_StreamInfo info;
            info.size = stream_size;
            if (ATX_BASE(self, BLT_BaseMediaNode).context) {
                info.mask = BLT_STREAM_INFO_MASK_SIZE;
                BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &info);
            }
        }
    } else {
        /* keep a reference to this stream as-is */
        self->input.stream = stream;
        ATX_REFERENCE_OBJECT(stream);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    TagParserInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
TagParserInput_QueryMediaType(BLT_MediaPort*        _self,
                              BLT_Ordinal           index,
                              const BLT_MediaType** media_type)
{
    TagParser* self = ATX_SELF_M(input, TagParser, BLT_MediaPort);

    if (index == 0) {
        *media_type = &self->input.media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(TagParserInput)
    ATX_GET_INTERFACE_ACCEPT(TagParserInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(TagParserInput, BLT_InputStreamUser)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_InputStreamUser interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(TagParserInput, BLT_InputStreamUser)
    TagParserInput_SetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(TagParserInput, 
                                         "input",
                                         STREAM_PULL,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(TagParserInput, BLT_MediaPort)
    TagParserInput_GetName,
    TagParserInput_GetProtocol,
    TagParserInput_GetDirection,
    TagParserInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    TagParserOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
TagParserOutput_QueryMediaType(BLT_MediaPort*        _self,
                               BLT_Ordinal           index,
                               const BLT_MediaType** media_type)
{
    TagParser* self = ATX_SELF_M(output, TagParser, BLT_MediaPort);

    if (index == 0) {
        *media_type = (BLT_MediaType*)&self->input.media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   TagParserOutput_GetStream
+---------------------------------------------------------------------*/
BLT_METHOD
TagParserOutput_GetStream(BLT_InputStreamProvider* _self,
                          ATX_InputStream**        stream)
{
    TagParser* self = ATX_SELF_M(output, TagParser, BLT_InputStreamProvider);

    /* return the stream */
    *stream = self->input.stream;
    ATX_REFERENCE_OBJECT(*stream);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(TagParserOutput)
    ATX_GET_INTERFACE_ACCEPT(TagParserOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(TagParserOutput, BLT_InputStreamProvider)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(TagParserOutput, 
                                         "output",
                                         STREAM_PULL,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(TagParserOutput, BLT_MediaPort)
    TagParserOutput_GetName,
    TagParserOutput_GetProtocol,
    TagParserOutput_GetDirection,
    TagParserOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_InputStreamProvider interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(TagParserOutput, BLT_InputStreamProvider)
    TagParserOutput_GetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    TagParser_Create
+---------------------------------------------------------------------*/
static BLT_Result
TagParser_Create(BLT_Module*              module,
                 BLT_Core*                core, 
                 BLT_ModuleParametersType parameters_type,
                 BLT_CString              parameters, 
                 BLT_MediaNode**          object)
{
    TagParser* parser;

    ATX_LOG_FINE("TagParser::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        *object = NULL;
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    parser = ATX_AllocateZeroMemory(sizeof(TagParser));
    if (parser == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(parser, BLT_BaseMediaNode), module, core);

    /* construct the object */
    BLT_MediaType_Init(&parser->input.media_type, 
                       ((TagParserModule*)module)->mpeg_audio_type_id);
    parser->input.stream = NULL;
    parser->input.media_type = parser->input.media_type;
    parser->input.media_type.flags = BLT_TAG_PARSER_MEDIA_TYPE_FLAGS_PARSED;

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(parser, TagParser, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(parser, TagParser, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&parser->input,  TagParserInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&parser->input,  TagParserInput,  BLT_InputStreamUser);
    ATX_SET_INTERFACE(&parser->output, TagParserOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&parser->output, TagParserOutput, BLT_InputStreamProvider);
    *object = &ATX_BASE_EX(parser, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    TagParser_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
TagParser_Destroy(TagParser* self)
{
    ATX_LOG_FINE("TagParser::Destroy");

    /* release the reference to the stream */
    ATX_RELEASE_OBJECT(self->input.stream);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    TagParser_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
TagParser_Deactivate(BLT_MediaNode* _self)
{
    TagParser* self = ATX_SELF_EX(TagParser, BLT_BaseMediaNode, BLT_MediaNode);

    ATX_LOG_FINER("TagParser::Deactivate");

    /* call the base class method */
    BLT_BaseMediaNode_Deactivate(_self);

    /* release the input stream */
    ATX_RELEASE_OBJECT(self->input.stream);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   TagParser_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
TagParser_GetPortByName(BLT_MediaNode*  _self,
                        BLT_CString     name,
                        BLT_MediaPort** port)
{
    TagParser* self = ATX_SELF_EX(TagParser, BLT_BaseMediaNode, BLT_MediaNode);

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
|    TagParser_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
TagParser_Seek(BLT_MediaNode* _self,
               BLT_SeekMode*  mode,
               BLT_SeekPoint* point)
{
    TagParser* self = ATX_SELF_EX(TagParser, BLT_BaseMediaNode, BLT_MediaNode);
    BLT_Result result;

    /* estimate the seek offset from the other stream parameters */
    result = BLT_Stream_EstimateSeekPoint(ATX_BASE(self, BLT_BaseMediaNode).context, *mode, point);
    if (BLT_FAILED(result)) return result;
    if (!(point->mask & BLT_SEEK_POINT_MASK_OFFSET)) {
        return BLT_FAILURE;
    }

    ATX_LOG_FINER_1("TagParser::Seek - seek offset = %d", (int)point->offset);

    /* seek into the input stream (ignore return value) */
    ATX_InputStream_Seek(self->input.stream, point->offset);

    /* set the mode so that the nodes down the chain know the seek has */
    /* already been done on the stream                                 */
    *mode = BLT_SEEK_MODE_IGNORE;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(TagParser)
    ATX_GET_INTERFACE_ACCEPT_EX(TagParser, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(TagParser, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(TagParser, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    TagParser_GetPortByName,
    BLT_BaseMediaNode_Activate,
    TagParser_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    TagParser_Seek
};

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(TagParser, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   TagParserModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
TagParserModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    TagParserModule* self = ATX_SELF_EX(TagParserModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*    registry;
    BLT_Result       result;

    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the ".mp3" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".mp3",
                                            "audio/mpeg");
    if (BLT_FAILED(result)) return result;

    /* get the type id for "audio/mpeg" */
    result = BLT_Registry_GetIdForName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/mpeg",
        &self->mpeg_audio_type_id);
    if (BLT_FAILED(result)) return result;
    
    ATX_LOG_FINE_1("TagParserModule::Attach (audio/mpeg type = %d)", self->mpeg_audio_type_id);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   TagParserModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
TagParserModule_Probe(BLT_Module*              _self, 
                      BLT_Core*                core,
                      BLT_ModuleParametersType parameters_type,
                      BLT_AnyConst             parameters,
                      BLT_Cardinal*            match)
{
    TagParserModule* self = ATX_SELF_EX(TagParserModule, BLT_BaseModule, BLT_Module);
    BLT_COMPILER_UNUSED(core);

    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

            /* we need the input protocol to b STREAM_PULL and the output */
            /* protocol to be STREAM_PULL                                 */
            if ((constructor->spec.input.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL) ||
                (constructor->spec.output.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL)) {
                return BLT_FAILURE;
            }

            /* the input type should be 'audio/mpeg' */
            if (constructor->spec.input.media_type->id != 
                self->mpeg_audio_type_id) {
                return BLT_FAILURE;
            }

            /* refuse to parse "parsed" streams (i.e streams that we have */
            /* already parsed                                             */
            if (constructor->spec.input.media_type->flags &
                BLT_TAG_PARSER_MEDIA_TYPE_FLAGS_PARSED) {
                ATX_LOG_FINE("TagParserModule::Probe - Already parsed");
                return BLT_FAILURE;
            }

            /* the output type should be unspecififed or audio/mpeg */
            if (!(constructor->spec.output.media_type->id ==
                  BLT_MEDIA_TYPE_ID_UNKNOWN) &&
                !(constructor->spec.output.media_type->id ==
                  self->mpeg_audio_type_id)) {
                return BLT_FAILURE;
            }

            /* compute the match level */
            if (constructor->name != NULL) {
                /* we're being probed by name */
                if (ATX_StringsEqual(constructor->name, "TagParser")) {
                    /* our name */
                    *match = BLT_MODULE_PROBE_MATCH_EXACT;
                } else {
                    /* not our name */
                    return BLT_FAILURE;
                }
            } else {
                /* we're probed by protocol/type specs only */
                *match = BLT_MODULE_PROBE_MATCH_MAX - 50;
            }

            ATX_LOG_FINE_1("TagParserModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(TagParserModule)
    ATX_GET_INTERFACE_ACCEPT_EX(TagParserModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(TagParserModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(TagParserModule, TagParser)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(TagParserModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    TagParserModule_Attach,
    TagParserModule_CreateInstance,
    TagParserModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define TagParserModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(TagParserModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(TagParserModule,
                                         "Tag Parser",
                                         "com.axiosys.parser.tags",
                                         "1.0.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)

