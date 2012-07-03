/*****************************************************************
|
|   Wave Parser Module
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
#include "BltWaveParser.h"
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
ATX_SET_LOCAL_LOGGER("bluetune.plugins.parsers.wave")

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_UInt32 wav_type_id;
} WaveParserModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamUser);

    /* members */
    BLT_MediaType media_type;
} WaveParserInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamProvider);

    /* members */
    ATX_InputStream* stream;
    BLT_Size         size;
    BLT_MediaType*   media_type;
    unsigned int     block_size;
} WaveParserOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    WaveParserInput  input;
    WaveParserOutput output;
} WaveParser;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_WAVE_HEADER_BUFFER_SIZE      32
#define BLT_WAVE_HEADER_RIFF_LOOKUP_SIZE 12
#define BLT_WAVE_HEADER_FMT_LOOKUP_SIZE  16
#define BLT_WAVE_HEADER_MAX_LOOKUP       524288

/*----------------------------------------------------------------------
|   WAVE tags
+---------------------------------------------------------------------*/
#define BLT_WAVE_FORMAT_UNKNOWN            0x0000
#define BLT_WAVE_FORMAT_PCM                0x0001
#define BLT_WAVE_FORMAT_IEEE_FLOAT         0x0003
#define BLT_WAVE_FORMAT_ALAW               0x0006
#define BLT_WAVE_FORMAT_MULAW              0x0007
#define BLT_WAVE_FORMAT_MPEG               0x0050
#define BLT_WAVE_FORMAT_MPEGLAYER3         0x0055
#define BLT_WAVE_FORMAT_EXTENSIBLE         0xFFFE
#define BLT_WAVE_FORMAT_DEVELOPMENT        0xFFFF


static const unsigned char BLT_WAVE_KS_DATA_FORMAT_SUBTYPE_PCM[16]  =
{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71};
static const unsigned char BLT_WAVE_KS_DATA_FORMAT_SUBTYPE_IEEE_FLOAT[16]  =
{0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71};

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(WaveParserModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(WaveParser, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(WaveParser, ATX_Referenceable)

/*----------------------------------------------------------------------
|   WaveParser_ParseHeader
+---------------------------------------------------------------------*/
BLT_METHOD
WaveParser_ParseHeader(WaveParser*      self, 
                       ATX_InputStream* stream,
                       BLT_Size*        header_size,
                       BLT_StreamInfo*  stream_info)
{
    unsigned char buffer[BLT_WAVE_HEADER_BUFFER_SIZE];
    BLT_Offset    position;
    BLT_Cardinal  bytes_per_second = 0;
    BLT_Result    result;
    
    /* check that we have a stream */
    if (stream == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* rewind the byte stream */
    ATX_InputStream_Seek(stream, 0);
    position = 0;
    *header_size = 0;

    /* no initial info yet */
    stream_info->mask = 0;

    /* read the wave header */
    result = ATX_InputStream_ReadFully(stream, buffer, 
                                       BLT_WAVE_HEADER_RIFF_LOOKUP_SIZE);
    if (BLT_FAILED(result)) {
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }
    position += BLT_WAVE_HEADER_RIFF_LOOKUP_SIZE;

    /* parse the header */
    if (buffer[0] != 'R' ||
        buffer[1] != 'I' ||
        buffer[2] != 'F' ||
        buffer[3] != 'F') {
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }

    if (buffer[ 8] != 'W' ||
        buffer[ 9] != 'A' ||
        buffer[10] != 'V' ||
        buffer[11] != 'E') {
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }

    do {
        unsigned long chunk_size;
        unsigned int  format_tag;
        result = ATX_InputStream_ReadFully(stream, buffer, 8);
        if (BLT_FAILED(result)) {
            return BLT_ERROR_INVALID_MEDIA_FORMAT;
        }
        position += 8;
        chunk_size = ATX_BytesToInt32Le(buffer+4);

        if (buffer[0] == 'f' &&
            buffer[1] == 'm' &&
            buffer[2] == 't' &&
            buffer[3] == ' ') {
            /* 'fmt ' chunk */
            result = ATX_InputStream_ReadFully(stream, buffer, 
                                               BLT_WAVE_HEADER_FMT_LOOKUP_SIZE);
            if (BLT_FAILED(result)) {
                return BLT_ERROR_INVALID_MEDIA_FORMAT;
            }
            position += BLT_WAVE_HEADER_FMT_LOOKUP_SIZE;

            format_tag = ATX_BytesToInt16Le(buffer);
            switch (format_tag) {
              case BLT_WAVE_FORMAT_PCM:
              case BLT_WAVE_FORMAT_IEEE_FLOAT: {
                    /* read the media type */
                    BLT_PcmMediaType media_type;
                    BLT_PcmMediaType_Init(&media_type);
                    BLT_MediaType_Free(self->output.media_type);
                    media_type.channel_count   = ATX_BytesToInt16Le(buffer+2);
                    media_type.channel_mask    = 0;
                    media_type.sample_rate     = ATX_BytesToInt32Le(buffer+4);
                    media_type.bits_per_sample = (BLT_UInt8)(8*((ATX_BytesToInt16Le(buffer+14)+7)/8));
                    if (format_tag == BLT_WAVE_FORMAT_IEEE_FLOAT) {
                        media_type.sample_format = BLT_PCM_SAMPLE_FORMAT_FLOAT_LE;
                    } else {
                        if (media_type.bits_per_sample == 8) {
                            /* 8 bits is always unsigned */
                            media_type.sample_format = BLT_PCM_SAMPLE_FORMAT_UNSIGNED_INT_LE;
                        } else {
                            /* more than 8 bits is always signed */
                            media_type.sample_format = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_LE;
                        }
                    }
                    BLT_MediaType_Clone((const BLT_MediaType*)&media_type, &self->output.media_type);

                    /* compute the block size */
                    self->output.block_size = media_type.channel_count*media_type.bits_per_sample/8;

                    /* update the stream info */
                    stream_info->sample_rate   = media_type.sample_rate;
                    stream_info->channel_count = media_type.channel_count;
                    stream_info->data_type     = "PCM";
                    stream_info->mask |=
                        BLT_STREAM_INFO_MASK_SAMPLE_RATE   |
                        BLT_STREAM_INFO_MASK_CHANNEL_COUNT |
                        BLT_STREAM_INFO_MASK_DATA_TYPE;
                    bytes_per_second = 
                        media_type.channel_count *
                        media_type.sample_rate   *
                        media_type.bits_per_sample/8;
                    
                    break;
              }

              case BLT_WAVE_FORMAT_EXTENSIBLE: {
                    /* read the media type */
                    unsigned char extra[24];
                    BLT_PcmMediaType media_type;

                    if (chunk_size < BLT_WAVE_HEADER_FMT_LOOKUP_SIZE+sizeof(extra)) {
                        return BLT_ERROR_INVALID_MEDIA_FORMAT;
                    }
                    result = ATX_InputStream_ReadFully(stream, extra, sizeof(extra));
                    if (BLT_FAILED(result)) {
                        return BLT_ERROR_INVALID_MEDIA_FORMAT;
                    }
                    /* check the cbSize field */
                    if (ATX_BytesToInt16Le(extra) != 22) return BLT_ERROR_INVALID_MEDIA_FORMAT;
                    
                    BLT_PcmMediaType_Init(&media_type);
                    BLT_MediaType_Free(self->output.media_type);
                    media_type.channel_count   = ATX_BytesToInt16Le(buffer+2);
                    media_type.sample_rate     = ATX_BytesToInt32Le(buffer+4);
                    media_type.bits_per_sample = (BLT_UInt8)(8*((ATX_BytesToInt16Le(buffer+14)+7)/8));
                    media_type.channel_mask = ATX_BytesToInt32Le(&extra[4]);;
                    
                    /* look for a known subformat */
                    if (ATX_MemoryEqual(&extra[8], BLT_WAVE_KS_DATA_FORMAT_SUBTYPE_PCM, 16)) {
                        media_type.sample_format = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_LE;
                    } else if (ATX_MemoryEqual(&extra[8], BLT_WAVE_KS_DATA_FORMAT_SUBTYPE_IEEE_FLOAT, 16)) {
                        media_type.sample_format = BLT_PCM_SAMPLE_FORMAT_FLOAT_LE;
                    } else {
                        return BLT_ERROR_UNSUPPORTED_CODEC;
                    }
                    
                    BLT_MediaType_Clone((const BLT_MediaType*)&media_type, &self->output.media_type);

                    /* compute the block size */
                    self->output.block_size = media_type.channel_count*media_type.bits_per_sample/8;

                    /* update the stream info */
                    stream_info->sample_rate   = media_type.sample_rate;
                    stream_info->channel_count = media_type.channel_count;
                    stream_info->data_type     = "PCM";
                    stream_info->mask |=
                        BLT_STREAM_INFO_MASK_SAMPLE_RATE   |
                        BLT_STREAM_INFO_MASK_CHANNEL_COUNT |
                        BLT_STREAM_INFO_MASK_DATA_TYPE;
                    bytes_per_second = 
                        media_type.channel_count *
                        media_type.sample_rate   *
                        media_type.bits_per_sample/8;
                    
                    break;                
              }
              
              
              default:
                return BLT_ERROR_UNSUPPORTED_CODEC;
            }

            chunk_size -= BLT_WAVE_HEADER_FMT_LOOKUP_SIZE;
        } else if (buffer[0] == 'd' &&
                   buffer[1] == 'a' &&
                   buffer[2] == 't' &&
                   buffer[3] == 'a') {
            /* 'data' chunk */
            self->output.size = chunk_size;
            *header_size = position;

            /* compute stream info */
            if (chunk_size) {
                stream_info->size = chunk_size;
            } else {
                ATX_LargeSize stream_size = 0;
                ATX_InputStream_GetSize(stream, &stream_size);
                stream_info->size = stream_size;
            }
            stream_info->mask |= BLT_STREAM_INFO_MASK_SIZE;
            if (stream_info->size != 0 && bytes_per_second != 0) {
                stream_info->duration = (((ATX_UInt64)stream_info->size*1000)/bytes_per_second);
                stream_info->mask |= BLT_STREAM_INFO_MASK_DURATION;
            } else {
                stream_info->duration = 0;
            }
            stream_info->nominal_bitrate = 
            stream_info->average_bitrate = 
            stream_info->instant_bitrate = bytes_per_second*8;
            stream_info->mask |= 
                BLT_STREAM_INFO_MASK_NOMINAL_BITRATE |
                BLT_STREAM_INFO_MASK_AVERAGE_BITRATE |
                BLT_STREAM_INFO_MASK_INSTANT_BITRATE;
            return BLT_SUCCESS;
        }

        /* skip chunk */
        ATX_InputStream_Seek(stream, chunk_size+position);
        position = chunk_size+position;
    } while (position < BLT_WAVE_HEADER_MAX_LOOKUP);

    return BLT_ERROR_INVALID_MEDIA_FORMAT;
}

/*----------------------------------------------------------------------
|   WaveParserInput_SetStream
+---------------------------------------------------------------------*/
BLT_METHOD
WaveParserInput_SetStream(BLT_InputStreamUser* _self,
                          ATX_InputStream*     stream,
                          const BLT_MediaType* media_type)
{
    WaveParser*    self = ATX_SELF_M(input, WaveParser, BLT_InputStreamUser);
    BLT_Size       header_size;
    BLT_StreamInfo stream_info;
    BLT_Result     result;

    /* check media type */
    if (media_type == NULL || media_type->id != self->input.media_type.id) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* if we had a stream, release it */
    ATX_RELEASE_OBJECT(self->output.stream);

    /* parse the stream header */
    result = WaveParser_ParseHeader(self, 
                                    stream, 
                                    &header_size,
                                    &stream_info);
    if (BLT_FAILED(result)) return result;

    /* update the stream info */
    if (stream_info.mask && ATX_BASE(self, BLT_BaseMediaNode).context) {
        BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, 
                           &stream_info);
    }

    /* create a substream */
    return ATX_SubInputStream_Create(stream, 
                                     header_size, 
                                     self->output.size,
                                     NULL,
                                     &self->output.stream);
}

/*----------------------------------------------------------------------
|   WaveParserInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
WaveParserInput_QueryMediaType(BLT_MediaPort*        _self,
                               BLT_Ordinal           index,
                               const BLT_MediaType** media_type)
{
    WaveParser* self = ATX_SELF_M(input, WaveParser, BLT_MediaPort);
    
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(WaveParserInput)
    ATX_GET_INTERFACE_ACCEPT(WaveParserInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(WaveParserInput, BLT_InputStreamUser)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_InputStreamUser interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(WaveParserInput, BLT_InputStreamUser)
    WaveParserInput_SetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(WaveParserInput, 
                                         "input",
                                         STREAM_PULL,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(WaveParserInput, BLT_MediaPort)
    WaveParserInput_GetName,
    WaveParserInput_GetProtocol,
    WaveParserInput_GetDirection,
    WaveParserInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   WaveParserOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
WaveParserOutput_QueryMediaType(BLT_MediaPort*        _self,
                                BLT_Ordinal           index,
                                const BLT_MediaType** media_type)
{
    WaveParser* self = ATX_SELF_M(output, WaveParser, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = self->output.media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   WaveParserOutput_GetStream
+---------------------------------------------------------------------*/
BLT_METHOD
WaveParserOutput_GetStream(BLT_InputStreamProvider* _self,
                           ATX_InputStream**        stream)
{
    WaveParser* self = ATX_SELF_M(output, WaveParser, BLT_InputStreamProvider);

    *stream = self->output.stream;
    ATX_REFERENCE_OBJECT(*stream);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(WaveParserOutput)
    ATX_GET_INTERFACE_ACCEPT(WaveParserOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(WaveParserOutput, BLT_InputStreamProvider)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(WaveParserOutput, 
                                         "output",
                                         STREAM_PULL,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(WaveParserOutput, BLT_MediaPort)
    WaveParserOutput_GetName,
    WaveParserOutput_GetProtocol,
    WaveParserOutput_GetDirection,
    WaveParserOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_InputStreamProvider interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(WaveParserOutput, BLT_InputStreamProvider)
    WaveParserOutput_GetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   WaveParser_Create
+---------------------------------------------------------------------*/
static BLT_Result
WaveParser_Create(BLT_Module*              module,
                  BLT_Core*                core, 
                  BLT_ModuleParametersType parameters_type,
                  BLT_CString              parameters, 
                  BLT_MediaNode**          object)
{
    WaveParser* self;

    ATX_LOG_FINE("WaveParser::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(WaveParser));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* construct the object */
    BLT_MediaType_Init(&self->input.media_type,
                       ((WaveParserModule*)module)->wav_type_id);
    self->output.stream = NULL;

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, WaveParser, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, WaveParser, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  WaveParserInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  WaveParserInput,  BLT_InputStreamUser);
    ATX_SET_INTERFACE(&self->output, WaveParserOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, WaveParserOutput, BLT_InputStreamProvider);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WaveParser_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
WaveParser_Destroy(WaveParser* self)
{
    ATX_LOG_FINE("WaveParser::Destroy");

    /* release the byte stream */
    ATX_RELEASE_OBJECT(self->output.stream);

    /* free the media type extensions */
    BLT_MediaType_Free(self->output.media_type);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    WaveParser_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
WaveParser_Deactivate(BLT_MediaNode* _self)
{
    WaveParser* self = ATX_SELF_EX(WaveParser, BLT_BaseMediaNode, BLT_MediaNode);

    ATX_LOG_FINER("WaveParser::Deactivate");

    /* release the stream */
    ATX_RELEASE_OBJECT(self->output.stream);

    /* call the base class method */
    BLT_BaseMediaNode_Deactivate(_self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WaveParser_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
WaveParser_GetPortByName(BLT_MediaNode*  _self,
                         BLT_CString     name,
                         BLT_MediaPort** port)
{
    WaveParser* self = ATX_SELF_EX(WaveParser, BLT_BaseMediaNode, BLT_MediaNode);

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
|   WaveParser_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
WaveParser_Seek(BLT_MediaNode* _self,
                BLT_SeekMode*  mode,
                BLT_SeekPoint* point)
{
    WaveParser* self = ATX_SELF_EX(WaveParser, BLT_BaseMediaNode, BLT_MediaNode);

    /* estimate the seek point */
    if (ATX_BASE(self, BLT_BaseMediaNode).context == NULL) return BLT_FAILURE;
    BLT_Stream_EstimateSeekPoint(ATX_BASE(self, BLT_BaseMediaNode).context, *mode, point);
    if (!(point->mask & BLT_SEEK_POINT_MASK_OFFSET)) {
        return BLT_FAILURE;
    }

    /* align the offset to the nearest sample */
    point->offset -= point->offset%(self->output.block_size);

    /* seek to the estimated offset */
    /* seek into the input stream (ignore return value) */
    ATX_InputStream_Seek(self->output.stream, point->offset);
    
    /* set the mode so that the nodes down the chain know the seek has */
    /* already been done on the stream                                  */
    *mode = BLT_SEEK_MODE_IGNORE;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(WaveParser)
    ATX_GET_INTERFACE_ACCEPT_EX(WaveParser, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(WaveParser, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(WaveParser, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    WaveParser_GetPortByName,
    BLT_BaseMediaNode_Activate,
    WaveParser_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    WaveParser_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(WaveParser, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   WaveParserModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
WaveParserModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    WaveParserModule* self = ATX_SELF_EX(WaveParserModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*     registry;
    BLT_Result        result;

    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the ".wav" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".wav",
                                            "audio/wav");
    if (BLT_FAILED(result)) return result;

    /* get the type id for "audio/wav" */
    result = BLT_Registry_GetIdForName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/wav",
        &self->wav_type_id);
    if (BLT_FAILED(result)) return result;
    
    ATX_LOG_FINE_1("Wave Parser Module::Attach (audio/wav type = %d)", self->wav_type_id);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WaveParserModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
WaveParserModule_Probe(BLT_Module*              _self, 
                       BLT_Core*                core,
                       BLT_ModuleParametersType parameters_type,
                       BLT_AnyConst             parameters,
                       BLT_Cardinal*            match)
{
    WaveParserModule* self = ATX_SELF_EX(WaveParserModule, BLT_BaseModule, BLT_Module);
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

            /* we need the input media type to be 'audio/wav' */
            if (constructor->spec.input.media_type->id != self->wav_type_id) {
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
                if (ATX_StringsEqual(constructor->name, "WaveParser")) {
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

            ATX_LOG_FINE_1("WaveParserModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(WaveParserModule)
    ATX_GET_INTERFACE_ACCEPT_EX(WaveParserModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(WaveParserModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(WaveParserModule, WaveParser)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(WaveParserModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    WaveParserModule_Attach,
    WaveParserModule_CreateInstance,
    WaveParserModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define WaveParserModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(WaveParserModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(WaveParserModule,
                                         "WAVE Parser",
                                         "com.axiosys.parser.wave",
                                         "1.0.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
