/*****************************************************************
|
|   BlueTune - AIFF Parser Module
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
#include "BltAiffParser.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltByteStreamProvider.h"
#include "BltByteStreamUser.h"
#include "BltStream.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.parsers.aiff")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef unsigned long AiffChunkType;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_UInt32 aiff_type_id;
} AiffParserModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamUser);

    /* members */
    BLT_MediaType media_type;
} AiffParserInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamProvider);

    /* members */
    ATX_InputStream* stream;
    BLT_LargeSize    size;
    BLT_PcmMediaType media_type;
} AiffParserOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    AiffParserInput  input;
    AiffParserOutput output;
} AiffParser;

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define BLT_AIFF_PARSER_PACKET_SIZE      4096
#define BLT_AIFF_CHUNK_HEADER_SIZE       8

#define BLT_AIFF_CHUNK_TYPE_FORM         0x464F524D  /* 'FORM' */
#define BLT_AIFF_CHUNK_TYPE_COMM         0x434F4D4D  /* 'COMM' */
#define BLT_AIFF_CHUNK_TYPE_SSND         0x53534E44  /* 'SSND' */

#define BLT_AIFF_COMM_CHUNK_MIN_DATA_SIZE  18
#define BLT_AIFC_COMM_CHUNK_MIN_DATA_SIZE  22
#define BLT_AIFF_SSND_CHUNK_MIN_SIZE       8

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(AiffParserModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(AiffParser, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(AiffParser, ATX_Referenceable)

/*----------------------------------------------------------------------
|   Aiff_ReadChunkHeader
+---------------------------------------------------------------------*/
static BLT_Result
Aiff_ReadChunkHeader(ATX_InputStream* stream,
                     AiffChunkType*   chunk_type,
                     ATX_Size*        chunk_size,
                     ATX_Size*        data_size)
{
    unsigned char buffer[BLT_AIFF_CHUNK_HEADER_SIZE];
    ATX_Result    result;

    /* try to read one header */
    result = ATX_InputStream_ReadFully(stream, buffer, sizeof(buffer));
    if (BLT_FAILED(result)) return result;
    
    /* parse chunk type */
    *chunk_type = ATX_BytesToInt32Be(buffer);

    /* compute data size */
    *data_size = ATX_BytesToInt32Be(&buffer[4]);
    
    /* the chunk size is the data size plus padding */
    *chunk_size = *data_size + ((*data_size)&0x1);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   Aiff_ParseExtended
+---------------------------------------------------------------------*/
static unsigned long 
Aiff_ParseExtended(unsigned char * buffer)
{
   unsigned long mantissa;
   unsigned long last = 0;
   unsigned char exp;

   mantissa = ATX_BytesToInt32Be(buffer+2);
   exp = 30 - *(buffer+1);
   while (exp--) {
       last = mantissa;
       mantissa >>= 1;
   }
   if (last & 0x00000001) mantissa++;
   return mantissa;
}

/*----------------------------------------------------------------------
|   AiffParser_OnCommChunk
+---------------------------------------------------------------------*/
static BLT_Result
AiffParser_OnCommChunk(AiffParser*      self, 
                       ATX_InputStream* stream, 
                       ATX_Size         data_size, 
                       BLT_Boolean      is_aifc)
{
    unsigned char  comm_buffer[BLT_AIFC_COMM_CHUNK_MIN_DATA_SIZE];
    unsigned short num_channels;
    unsigned long  num_sample_frames;
    unsigned short sample_size;
    unsigned long  sample_rate;
    BLT_Size       header_size;
    BLT_Boolean    is_float = BLT_FALSE;
    BLT_Result     result;

    /* read the chunk data */
    if (is_aifc) {
        header_size = BLT_AIFC_COMM_CHUNK_MIN_DATA_SIZE;
    } else {
        header_size = BLT_AIFF_COMM_CHUNK_MIN_DATA_SIZE;
    }
    result = ATX_InputStream_ReadFully(stream, comm_buffer, header_size); 
    if (ATX_FAILED(result)) return result;

    /* skip any unread part */
    if (header_size != data_size) {
        ATX_InputStream_Skip(stream, data_size-header_size);
    }

    /* parse the fields */
    num_channels      = ATX_BytesToInt16Be(comm_buffer);
    num_sample_frames = ATX_BytesToInt32Be(&comm_buffer[2]);
    sample_size       = ATX_BytesToInt16Be(&comm_buffer[6]);
    sample_rate       = Aiff_ParseExtended(&comm_buffer[8]);

    /* check the compression type for AIFC chunks */
    if (is_aifc) {
        if (comm_buffer[18] == 'N' &&
            comm_buffer[19] == 'O' &&
            comm_buffer[20] == 'N' &&
            comm_buffer[21] == 'E') {
            is_float = BLT_FALSE;
        } else if (comm_buffer[18] == 'f' &&
                   comm_buffer[19] == 'l' &&
                   comm_buffer[20] == '3' &&
                   comm_buffer[21] == '2') {
            is_float = BLT_TRUE;
        } else {
            return BLT_ERROR_UNSUPPORTED_FORMAT;
        }
    }

    /* compute sample format */
    sample_size = 8*((sample_size+7)/8);
    if (is_float) {
        self->output.media_type.sample_format = BLT_PCM_SAMPLE_FORMAT_FLOAT_BE;
    } else {
        self->output.media_type.sample_format = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_BE;
    }

    /* update the format */
    self->output.media_type.channel_count   = num_channels;
    self->output.media_type.channel_mask    = 0;
    self->output.media_type.bits_per_sample = (BLT_UInt8)sample_size;
    self->output.media_type.sample_rate     = sample_rate;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AiffParser_OnSsndChunk
+---------------------------------------------------------------------*/
static BLT_Result
AiffParser_OnSsndChunk(AiffParser*      self, 
                       ATX_InputStream* stream, 
                       BLT_LargeSize    size)
{
    unsigned char ssnd_buffer[BLT_AIFF_SSND_CHUNK_MIN_SIZE];
    unsigned long offset;
    unsigned long block_size;
    ATX_Position  position;
    BLT_Result    result;

    /* read the chunk data */
    result = ATX_InputStream_ReadFully(stream, ssnd_buffer, sizeof(ssnd_buffer)); 
    if (ATX_FAILED(result)) return result;

    /* parse the fields */
    offset     = ATX_BytesToInt32Be(ssnd_buffer);
    block_size = ATX_BytesToInt32Be(&ssnd_buffer[4]);

    /* see where we are in the stream */
    result = ATX_InputStream_Tell(stream, &position);
    if (ATX_FAILED(result)) return result;

    /* create a sub stream */
    self->output.size = size;
    return ATX_SubInputStream_Create(stream, 
                                     position+offset, 
                                     size,
                                     NULL,
                                     &self->output.stream);
}

/*----------------------------------------------------------------------
|   AiffParser_ParseChunks
+---------------------------------------------------------------------*/
static BLT_Result
AiffParser_ParseChunks(AiffParser*      self, 
                       ATX_InputStream* stream)
{
    AiffChunkType  chunk_type;
    ATX_Size       chunk_size;
    ATX_Size       data_size;
    BLT_Boolean    have_comm = BLT_FALSE;
    BLT_Boolean    have_ssnd = BLT_FALSE;
    BLT_Boolean    is_aifc = BLT_FALSE;
    BLT_StreamInfo stream_info;
    int            bytes_per_second;
    BLT_Result     result;
    
    /* check that we have a stream */
    if (stream == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* rewind the byte stream */
    ATX_InputStream_Seek(stream, 0);

    /* get the first chunk header */
    result = Aiff_ReadChunkHeader(stream, 
                                  &chunk_type,
                                  &chunk_size, 
                                  &data_size);
    if (BLT_FAILED(result)) return result;

    /* we expect a 'FORM' chunk here */
    if (chunk_type != BLT_AIFF_CHUNK_TYPE_FORM) {
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }

    /* read the FORM type */
    {
        unsigned char type_buffer[4];
        result = ATX_InputStream_ReadFully(stream, type_buffer, 4);
        if (BLT_FAILED(result)) return result;
        
        /* we support 'AIFF' and 'AIFC' */
        if (type_buffer[0] == 'A' &&
            type_buffer[1] == 'I' &&
            type_buffer[2] == 'F') {
            if (type_buffer[3] == 'F') {
                is_aifc = BLT_FALSE;
            } else if (type_buffer[3] == 'C') { 
                is_aifc = BLT_TRUE;
            } else {
                return BLT_ERROR_INVALID_MEDIA_FORMAT;
            }
        } else {
            return BLT_ERROR_INVALID_MEDIA_FORMAT;
        }
    }

    /* iterate over all the chunks until we have found COMM and SSND */
    do {
        /* read the next chunk header */
        result = Aiff_ReadChunkHeader(stream, &chunk_type, &chunk_size, &data_size);
        if (BLT_FAILED(result)) return result;
        
        switch (chunk_type) {
            case BLT_AIFF_CHUNK_TYPE_COMM: 
                /* check the chunk size */
                if (data_size < (BLT_Size)(is_aifc ? BLT_AIFC_COMM_CHUNK_MIN_DATA_SIZE :
                                                     BLT_AIFF_COMM_CHUNK_MIN_DATA_SIZE)) {
                    return BLT_ERROR_UNSUPPORTED_FORMAT;
                }

                /* process the chunk */
                result = AiffParser_OnCommChunk(self, stream, data_size, is_aifc);
                if (BLT_FAILED(result)) return result;
                have_comm = BLT_TRUE;
                break;

            case BLT_AIFF_CHUNK_TYPE_SSND: 
                /* check the chunk size */
                if (data_size < BLT_AIFF_SSND_CHUNK_MIN_SIZE) {
                    if (data_size == 0) {
                        /* this could be caused by a software that wrote a */
                        /* temporary header and was not able to update it  */
                        ATX_LargeSize input_size;
                        ATX_Position  position;
                        ATX_InputStream_GetSize(stream, &input_size);
                        ATX_InputStream_Tell(stream, &position);
                        if (input_size > (ATX_Size)(position+8)) {
                            data_size = (ATX_Size)(input_size-position-8);
                        }
                    } else {
                        return BLT_ERROR_INVALID_MEDIA_FORMAT;
                    }
                }

                /* process the chunk */
                result = AiffParser_OnSsndChunk(self, stream, data_size);
                if (BLT_FAILED(result)) return result;
                have_ssnd = BLT_TRUE;
                
                /* stop now if we have all we need */
                if (have_comm) break;

                /* skip the data (this should usually not happen, as the 
                   SSND chunk is typically the last one */
                ATX_InputStream_Skip(stream, data_size-BLT_AIFF_SSND_CHUNK_MIN_SIZE);
                break;

            default:
                /* ignore the chunk */
                result = ATX_InputStream_Skip(stream, chunk_size);
                if (ATX_FAILED(result)) return result;
        }
    } while (!have_comm || !have_ssnd);

    /* update the stream info */
    stream_info.mask =
        BLT_STREAM_INFO_MASK_SIZE          |
        BLT_STREAM_INFO_MASK_SAMPLE_RATE   |
        BLT_STREAM_INFO_MASK_CHANNEL_COUNT |
        BLT_STREAM_INFO_MASK_DATA_TYPE;
    stream_info.sample_rate   = self->output.media_type.sample_rate;
    stream_info.channel_count = self->output.media_type.channel_count;
    stream_info.data_type     = "PCM";
    stream_info.size          = data_size;
    bytes_per_second = 
        self->output.media_type.channel_count *
        self->output.media_type.sample_rate   *
        self->output.media_type.bits_per_sample/8;

    if (stream_info.size != 0 && bytes_per_second != 0) {
        stream_info.duration = (((ATX_UInt64)stream_info.size*1000)/bytes_per_second);
        stream_info.mask |= BLT_STREAM_INFO_MASK_DURATION;
    } else {
        stream_info.duration = 0;
    }
    stream_info.nominal_bitrate = 
    stream_info.average_bitrate = 
    stream_info.instant_bitrate = bytes_per_second*8;
    stream_info.mask |= 
        BLT_STREAM_INFO_MASK_NOMINAL_BITRATE |
        BLT_STREAM_INFO_MASK_AVERAGE_BITRATE |
        BLT_STREAM_INFO_MASK_INSTANT_BITRATE;
    if (stream_info.mask && ATX_BASE(self, BLT_BaseMediaNode).context) {
        BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &stream_info);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AiffParserInput_SetStream
+---------------------------------------------------------------------*/
BLT_METHOD
AiffParserInput_SetStream(BLT_InputStreamUser* _self,
                          ATX_InputStream*     stream,
                          const BLT_MediaType* media_type)
{
    AiffParser* self = ATX_SELF_M(input, AiffParser, BLT_InputStreamUser);
    BLT_COMPILER_UNUSED(media_type);

    /* check media type */
    if (media_type == NULL || media_type->id != self->input.media_type.id) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* if we had a stream, release it */
    ATX_RELEASE_OBJECT(self->output.stream);

    /* parse the chunks */
    return AiffParser_ParseChunks(self, stream);
}

/*----------------------------------------------------------------------
|    AiffParserInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
AiffParserInput_QueryMediaType(BLT_MediaPort*        _self,
                               BLT_Ordinal           index,
                               const BLT_MediaType** media_type)
{
    AiffParser* self = ATX_SELF_M(input, AiffParser, BLT_MediaPort);
    
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AiffParserInput)
    ATX_GET_INTERFACE_ACCEPT(AiffParserInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(AiffParserInput, BLT_InputStreamUser)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_InputStreamUser interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AiffParserInput, BLT_InputStreamUser)
    AiffParserInput_SetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(AiffParserInput, 
                                         "input",
                                         STREAM_PULL,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(AiffParserInput, BLT_MediaPort)
    AiffParserInput_GetName,
    AiffParserInput_GetProtocol,
    AiffParserInput_GetDirection,
    AiffParserInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    AiffParserOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
AiffParserOutput_QueryMediaType(BLT_MediaPort*        _self,
                                BLT_Ordinal           index,
                                const BLT_MediaType** media_type)
{
    AiffParser* self = ATX_SELF_M(output, AiffParser, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = (BLT_MediaType*)&self->output.media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   AiffParserOutput_GetStream
+---------------------------------------------------------------------*/
BLT_METHOD
AiffParserOutput_GetStream(BLT_InputStreamProvider* _self,
                           ATX_InputStream**        stream)
{
    AiffParser* self = ATX_SELF_M(output, AiffParser, BLT_InputStreamProvider);

    /* ensure we're at the start of the stream */
    ATX_InputStream_Seek(self->output.stream, 0);

    /* return the stream */
    *stream = self->output.stream;
    ATX_REFERENCE_OBJECT(*stream);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AiffParserOutput)
    ATX_GET_INTERFACE_ACCEPT(AiffParserOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(AiffParserOutput, BLT_InputStreamProvider)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(AiffParserOutput, 
                                         "output",
                                         STREAM_PULL,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(AiffParserOutput, BLT_MediaPort)
    AiffParserOutput_GetName,
    AiffParserOutput_GetProtocol,
    AiffParserOutput_GetDirection,
    AiffParserOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_InputStreamProvider interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AiffParserOutput, BLT_InputStreamProvider)
    AiffParserOutput_GetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    AiffParser_Create
+---------------------------------------------------------------------*/
static BLT_Result
AiffParser_Create(BLT_Module*              module,
                  BLT_Core*                core, 
                  BLT_ModuleParametersType parameters_type,
                  BLT_CString              parameters, 
                  BLT_MediaNode**          object)
{
    AiffParser* parser;

    ATX_LOG_FINE("AiffParser::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        *object = NULL;
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    parser = ATX_AllocateZeroMemory(sizeof(AiffParser));
    if (parser == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(parser, BLT_BaseMediaNode), module, core);

    /* construct the object */
    BLT_MediaType_Init(&parser->input.media_type, 
                       ((AiffParserModule*)module)->aiff_type_id);
    parser->output.stream = NULL;
    BLT_PcmMediaType_Init(&parser->output.media_type);

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(parser, AiffParser, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(parser, AiffParser, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&parser->input,  AiffParserInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&parser->input,  AiffParserInput,  BLT_InputStreamUser);
    ATX_SET_INTERFACE(&parser->output, AiffParserOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&parser->output, AiffParserOutput, BLT_InputStreamProvider);
    *object = &ATX_BASE_EX(parser, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AiffParser_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
AiffParser_Destroy(AiffParser* self)
{
    ATX_LOG_FINE("AiffParser::Destroy");

    /* release the byte stream */
    ATX_RELEASE_OBJECT(self->output.stream);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AiffParser_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
AiffParser_Deactivate(BLT_MediaNode* _self)
{
    AiffParser* self = ATX_SELF_EX(AiffParser, BLT_BaseMediaNode, BLT_MediaNode);

    ATX_LOG_FINER("AiffParser::Deactivate");

    /* call the base class method */
    BLT_BaseMediaNode_Deactivate(_self);

    /* release the output stream */
    ATX_RELEASE_OBJECT(self->output.stream);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AiffParser_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
AiffParser_GetPortByName(BLT_MediaNode*  _self,
                         BLT_CString     name,
                         BLT_MediaPort** port)
{
    AiffParser* self = ATX_SELF_EX(AiffParser, BLT_BaseMediaNode, BLT_MediaNode);

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
|    AiffParser_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
AiffParser_Seek(BLT_MediaNode* _self,
                BLT_SeekMode*  mode,
                BLT_SeekPoint* point)
{
    AiffParser* self = ATX_SELF_EX(AiffParser, BLT_BaseMediaNode, BLT_MediaNode);

    /* estimate the seek point */
    if (ATX_BASE(self, BLT_BaseMediaNode).context == NULL) return BLT_FAILURE;
    BLT_Stream_EstimateSeekPoint(ATX_BASE(self, BLT_BaseMediaNode).context, *mode, point);
    if (!(point->mask & BLT_SEEK_POINT_MASK_OFFSET)) {
        return BLT_FAILURE;
    }

    /* align the offset to the nearest sample */
    point->offset -= point->offset%8;

    /* seek to the estimated offset */
    /* seek into the input stream (ignore return value) */
    ATX_InputStream_Seek(self->output.stream, point->offset);
    
    /* set the mode so that the nodes down the chaine know the seek has */
    /* already been done on the stream                                  */
    *mode = BLT_SEEK_MODE_IGNORE;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AiffParser)
    ATX_GET_INTERFACE_ACCEPT_EX(AiffParser, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(AiffParser, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(AiffParser, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    AiffParser_GetPortByName,
    BLT_BaseMediaNode_Activate,
    AiffParser_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    AiffParser_Seek
};

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(AiffParser, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   AiffParserModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
AiffParserModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    AiffParserModule* self = ATX_SELF_EX(AiffParserModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*     registry;
    BLT_Result        result;

    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the file extensions */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".aif",
                                            "audio/aiff");
    if (BLT_FAILED(result)) return result;
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".aiff",
                                            "audio/aiff");
    if (BLT_FAILED(result)) return result;
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".aifc",
                                            "audio/aiff");
    if (BLT_FAILED(result)) return result;

    /* get the type id for "audio/aiff" and "audio/x-aiff" */
    result = BLT_Registry_GetIdForName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/aiff",
        &self->aiff_type_id);
    if (BLT_FAILED(result)) return result;
    
    result = BLT_Registry_RegisterNameForId(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/x-aiff",
        self->aiff_type_id);
    if (BLT_FAILED(result)) return result;

    ATX_LOG_FINE_1("AiffParserModule::Attach (audio/aiff type = %d)",
                   self->aiff_type_id);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AiffParserModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
AiffParserModule_Probe(BLT_Module*              _self, 
                       BLT_Core*                core,
                       BLT_ModuleParametersType parameters_type,
                       BLT_AnyConst             parameters,
                       BLT_Cardinal*            match)
{
    AiffParserModule* self = ATX_SELF_EX(AiffParserModule, BLT_BaseModule, BLT_Module);
    BLT_COMPILER_UNUSED(core);

    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

            /* we need the input protocol to be STREAM_PULL and the output */
            /* protocol to be STREAM_PULL                                  */
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

            /* we need the input media type to be 'audio/aiff' or 'audio/x-aiff' */
            if (constructor->spec.input.media_type->id != self->aiff_type_id) {
                return BLT_FAILURE;
            }

            /* the output type should be unknown at this point */
            if (constructor->spec.output.media_type->id != 
                BLT_MEDIA_TYPE_ID_UNKNOWN) {
                return BLT_FAILURE;
            }

            /* compute the match level */
            if (constructor->name != NULL) {
                /* we're being probed by name */
                if (ATX_StringsEqual(constructor->name, "AiffParser")) {
                    /* our name */
                    *match = BLT_MODULE_PROBE_MATCH_EXACT;
                } else {
                    /* not out name */
                    return BLT_FAILURE;
                }
            } else {
                /* we're probed by protocol/type specs only */
                *match = BLT_MODULE_PROBE_MATCH_MAX - 10;
            }

            ATX_LOG_FINE_1("AiffParserModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AiffParserModule)
    ATX_GET_INTERFACE_ACCEPT_EX(AiffParserModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(AiffParserModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(AiffParserModule, AiffParser)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(AiffParserModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    AiffParserModule_Attach,
    AiffParserModule_CreateInstance,
    AiffParserModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define AiffParserModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(AiffParserModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(AiffParserModule,
                                         "AIFF Parser",
                                         "com.axiosys.parser.aiff",
                                         "1.0.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
