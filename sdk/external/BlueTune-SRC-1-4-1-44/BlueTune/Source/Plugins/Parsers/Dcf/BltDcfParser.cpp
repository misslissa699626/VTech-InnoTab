/*****************************************************************
|
|   DCF Parser Module
|
|   (c) 2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4.h"
#include "Ap4StreamCipher.h"
#include "Ap4AtomixAdapters.h"
#include "BltConfig.h"
#include "BltDcfParser.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltByteStreamProvider.h"
#include "BltByteStreamUser.h"
#include "BltStream.h"
#include "BltCommonMediaTypes.h"
#include "BltKeyManager.h"
#include "BltBento4Adapters.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.parsers.dcf")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int BLT_DCF_PARSER_MAX_HEADERS_LENGTH     = 262144; // 256k
const char* const  BLT_DCF_PARSER_ALGORITHM_ID_AES128CBC = "AES128CBC";
const char* const  BLT_DCF_PARSER_PADDING_RFC2630        = "RFC2630";

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
struct DcfParserModule {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_UInt32 dcf1_type_id;
    BLT_UInt32 dcf2_type_id;
};

// it is important to keep this structure a POD (no methods)
// because the strict compilers will not like use using
// the offsetof() macro necessary when using ATX_SELF()
typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamUser);

    /* members */
    BLT_MediaType           dcf1_media_type;
    BLT_MediaType           dcf2_media_type;
    char                    content_type[257];
    char                    content_uri[257];
    BLT_LargeSize           encrypted_size;
} DcfParserInput;

// it is important to keep this structure a POD (no methods)
// because the strict compilers will not like use using
// the offsetof() macro necessary when using ATX_SELF()
typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamProvider);

    /* members */
    ATX_InputStream* stream;
    BLT_LargeSize    size;
    BLT_MediaType    media_type;
} DcfParserOutput;

// it is important to keep this structure a POD (no methods)
// because the strict compilers will not like use using
// the offsetof() macro necessary when using ATX_SELF()
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    DcfParserInput          input;
    DcfParserOutput         output;
    BLT_KeyManager*         key_manager;
    AP4_BlockCipherFactory* cipher_factory;
} DcfParser;

/*----------------------------------------------------------------------
|   DcfParserInput_Construct
+---------------------------------------------------------------------*/
static void
DcfParserInput_Construct(DcfParserInput* self, BLT_Module* module)
{
    DcfParserModule* parser_module = (DcfParserModule*)module;
    BLT_MediaType_Init(&self->dcf1_media_type, parser_module->dcf1_type_id);
    BLT_MediaType_Init(&self->dcf2_media_type, parser_module->dcf2_type_id);
    self->encrypted_size  = 0;
}

/*----------------------------------------------------------------------
|   DcfParserInput_Destruct
+---------------------------------------------------------------------*/
static void
DcfParserInput_Destruct(DcfParserInput* /*self*/)
{
}

/*----------------------------------------------------------------------
|   DcfParser_ParseUintvar
+---------------------------------------------------------------------*/
static BLT_Result
DcfParser_ReadUintvar(ATX_InputStream* stream, 
                      ATX_UInt32&      value,
                      unsigned int&    var_length)
{
    value      = 0;
    var_length = 0;
    while (var_length < 5) {
        ATX_UInt8 byte;
        ATX_Result result = ATX_InputStream_ReadFully(stream, &byte, 1);
        ++var_length;
        if (ATX_FAILED(result)) return result;
        value = (value<<7) + (byte&0x7F);
        if ((byte & 0x80) == 0) return BLT_SUCCESS;
    }
    
    /* too many bytes */
    value = 0;
    return BLT_ERROR_INVALID_MEDIA_FORMAT;
}

/*----------------------------------------------------------------------
|   DcfParser_GetContentKey
+---------------------------------------------------------------------*/
static BLT_Result
DcfParser_GetContentKey(DcfParser*      self, 
                        const char*     content_id,
                        NPT_DataBuffer& key)
{
    // default
    key.SetDataSize(0);

    // check that we have a key manager
    if (self->key_manager == NULL) return BLT_ERROR_NO_MEDIA_KEY;

    // ask the key manager to resolve the key
    key.Reserve(1024);
    BLT_Size key_size = 1024;
    BLT_Result result = BLT_KeyManager_GetKeyByName(self->key_manager, content_id, key.UseData(), &key_size);
    if (BLT_FAILED(result)) {
        if (result == ATX_ERROR_NOT_ENOUGH_SPACE) {
            key.Reserve(key_size);
            result = BLT_KeyManager_GetKeyByName(self->key_manager, content_id, key.UseData(), &key_size);
        } else {
            return result;
        }
    }
    key.SetDataSize(key_size);
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   DcfParser_ParseV1Header
+---------------------------------------------------------------------*/
static BLT_Result
DcfParser_ParseV1Header(DcfParser* self, ATX_InputStream* stream)
{
    /* rewind the byte stream */
    ATX_InputStream_Seek(stream, 0);

    /* user a buffer for parsing */
    ATX_UInt8 buffer[3];
    
    /* read the first 3 fields */
    ATX_Result result;
    result = ATX_InputStream_ReadFully(stream, buffer, 3);
    if (ATX_FAILED(result)) return result;

    /* check the version */
    ATX_UInt8 version = buffer[0];
    if (version != 1) {
        ATX_LOG_FINE_1("unsupported DCF version (%d)", version);
        return BLT_ERROR_UNSUPPORTED_FORMAT;
    }
    
    /* read the content type */
    ATX_UInt8 content_type_length = buffer[1];
    self->input.content_type[content_type_length] = 0; // null-terminate
    result = ATX_InputStream_ReadFully(stream, self->input.content_type, content_type_length);
    if (ATX_FAILED(result)) return result;
    
    /* read the content URI */
    ATX_UInt8 content_uri_length = buffer[2];
    self->input.content_uri[content_uri_length] = 0; // null-terminate
    result = ATX_InputStream_ReadFully(stream, self->input.content_uri, content_uri_length);
    if (ATX_FAILED(result)) return result;
    
    /* read the variable-length fields */
    unsigned int var_length_1, var_length_2;
    ATX_UInt32 headers_length = 0;
    result = DcfParser_ReadUintvar(stream, headers_length, var_length_1);
    if (ATX_FAILED(result)) return result;
    ATX_UInt32 data_length = 0;
    result = DcfParser_ReadUintvar(stream, data_length, var_length_2);
    if (ATX_FAILED(result)) return result;

    /* check that the encrypted size makes sense */
    if (data_length < 32) return BLT_ERROR_INVALID_MEDIA_FORMAT;
    self->input.encrypted_size = data_length;
    
    /* get the content key */
    NPT_DataBuffer key;
    result = DcfParser_GetContentKey(self, self->input.content_uri, key);
    if (BLT_FAILED(result)) {
        ATX_LOG_FINE_2("GetKeyForContent(%s) returned %d", 
            self->input.content_uri, 
            result);
        return BLT_ERROR_NO_MEDIA_KEY;
    }

    /* read the headers */
    if (headers_length > BLT_DCF_PARSER_MAX_HEADERS_LENGTH) return BLT_ERROR_INVALID_MEDIA_FORMAT;
    char* headers = new char[headers_length+1];
    headers[headers_length] = '\0';
    result = ATX_InputStream_ReadFully(stream, headers, headers_length);
    if (ATX_FAILED(result)) {
        delete[] headers;
        return result;
    }
    
    /* as a first-order estimate, set the output size to the encrypted size minus the IV */
    self->output.size = self->input.encrypted_size-16;
        
    /* parse the headers */
    NPT_MemoryStream* headers_memory_stream = new NPT_MemoryStream(headers, headers_length);
    NPT_InputStreamReference headers_memory_stream_ref(headers_memory_stream);
    NPT_BufferedInputStream* headers_buffered_stream = 
        new NPT_BufferedInputStream(headers_memory_stream_ref);
    NPT_HttpHeaders content_headers;
    content_headers.Parse(*headers_buffered_stream);
    delete headers_buffered_stream;
    delete[] headers;
    
    /* find out about the encryption from the headers */
    const NPT_String* encryption_method = content_headers.GetHeaderValue("Encryption-Method");
    if (encryption_method == NULL) return BLT_ERROR_INVALID_MEDIA_FORMAT;
    
    /* check the encryption method */
    bool                           encryption_supported = false;
    NPT_Map<NPT_String,NPT_String> encryption_params;
    NPT_Size                       algorithm_id_length = 0;
    
    /* parse the algorithm id */
    int separator = encryption_method->Find(';', 0, true);
    if (separator > 0) {
        algorithm_id_length = separator;
        
        /* parse the params */
        result = NPT_ParseMimeParameters(((const char*)(*encryption_method))+separator+1, 
                                         encryption_params);
        if (NPT_FAILED(result)) {
            ATX_LOG_FINE_1("cannot parse Encryption-Method parameters (%s)", 
                           (const char*)encryption_method);
            return BLT_ERROR_INVALID_MEDIA_FORMAT;
        }

        /* parse the plaintext-length header, if present */
        NPT_String* plaintext_length = NULL;
        if (NPT_SUCCEEDED(encryption_params.Get("plaintext-length", plaintext_length))) {
            NPT_UInt64 value = 0;
            if (NPT_SUCCEEDED(plaintext_length->ToInteger64(value, true))) {
                self->output.size = value;
            }
        }
    } else {
        algorithm_id_length = encryption_method->GetLength();
    }       

    if (NPT_StringsEqualN((const char*)(*encryption_method),
                          BLT_DCF_PARSER_ALGORITHM_ID_AES128CBC,
                          algorithm_id_length)) {
        encryption_supported = true;
        NPT_String* padding = NULL;
        if (NPT_SUCCEEDED(encryption_params.Get("padding", padding))) {
            if (*padding == BLT_DCF_PARSER_PADDING_RFC2630) {
                encryption_supported = false; // hmmm, unknown padding
            }
        } 
    }

    if (!encryption_supported) {
        ATX_LOG_FINE_1("unsupported encryption format (%s)", encryption_method);
        return BLT_ERROR_UNSUPPORTED_FORMAT;
    }
    
    /* read the IV */
    AP4_UI08 iv[16];
    result = ATX_InputStream_ReadFully(stream, iv, sizeof(iv));
    if (BLT_FAILED(result)) return result;
    
    /* create a byte stream to represent the encrypted data */
    ATX_Size header_size = 3+content_type_length+content_uri_length+var_length_1+var_length_2+headers_length;
    ATX_InputStream* data_stream = NULL;
    ATX_SubInputStream_Create(stream, 
                              header_size+16, // skip the IV
                              self->input.encrypted_size-16, 
                              NULL, 
                              &data_stream);
    ATX_InputStream_To_AP4_ByteStream_Adapter* encrypted_stream = 
        new ATX_InputStream_To_AP4_ByteStream_Adapter(data_stream);
    ATX_RELEASE_OBJECT(data_stream);
    
    /* create a decrypting stream for the content */ // FIXME: temporary
    AP4_ByteStream* decrypting_stream = NULL;
    result = AP4_DecryptingStream::Create(AP4_BlockCipher::CBC,
                                          *encrypted_stream,
                                          self->output.size,
                                          iv, 16, 
                                          key.GetData(),
                                          key.GetDataSize(),
                                          self->cipher_factory,
                                          decrypting_stream);
    encrypted_stream->Release();
    if (AP4_FAILED(result)) return result;

    /* create a reverse adapter */
    result = AP4_ByteStream_To_ATX_InputStream_Adapter_Create(decrypting_stream, &self->output.stream);
    decrypting_stream->Release();

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   DcfParser_ParseV2Header
+---------------------------------------------------------------------*/
static BLT_Result
DcfParser_ParseV2Header(DcfParser* self, ATX_InputStream* stream)
{
    /* rewind the byte stream */
    ATX_InputStream_Seek(stream, 0);

    /* parse the atoms from the stream */
    AP4_ByteStream* mp4_stream = new ATX_InputStream_To_AP4_ByteStream_Adapter(stream);
    AP4_AtomParent  atoms;
    AP4_Result result = AP4_DefaultAtomFactory::Instance.CreateAtomsFromStream(*mp4_stream, atoms);
    mp4_stream->Release();
    
    AP4_ByteStream* decrypting_stream = NULL;
    AP4_ContainerAtom* odrm = dynamic_cast<AP4_ContainerAtom*>(atoms.GetChild(AP4_ATOM_TYPE_ODRM));
    if (odrm) {
        AP4_OdheAtom* odhe = dynamic_cast<AP4_OdheAtom*>(odrm->GetChild(AP4_ATOM_TYPE_ODHE));
        AP4_OddaAtom* odda = dynamic_cast<AP4_OddaAtom*>(odrm->GetChild(AP4_ATOM_TYPE_ODDA));
        if (odhe && odda) {
            const char* content_id = "";

            /* get the content ID */
            AP4_OhdrAtom* ohdr = dynamic_cast<AP4_OhdrAtom*>(odhe->GetChild(AP4_ATOM_TYPE_OHDR));
            if (ohdr) {
                content_id = ohdr->GetContentId().GetChars();
            }

            /* get the content key */
            NPT_DataBuffer key;
            result = DcfParser_GetContentKey(self, content_id, key);
            if (BLT_FAILED(result)) {
                ATX_LOG_FINE_2("GetKeyForContent(%s) returned %d", 
                               content_id, 
                               result);
                return BLT_ERROR_NO_MEDIA_KEY;
            }

            /* create the decrypting stream */
            result = AP4_OmaDcfAtomDecrypter::CreateDecryptingStream(*odrm,
                                                                     key.GetData(),
                                                                     key.GetDataSize(),
                                                                     self->cipher_factory,
                                                                     decrypting_stream);
            if (AP4_SUCCEEDED(result)) {
                /* update the content type */
                ATX_CopyStringN(self->input.content_type,
                                odhe->GetContentType().GetChars(),
                                sizeof(self->input.content_type));
                    
                /* update the encrypted size */
                self->input.encrypted_size = odda->GetEncryptedDataLength();
            }
        }
    }

    /* check that we have found what we needed in the atoms */
    if (decrypting_stream == NULL) return BLT_ERROR_INVALID_MEDIA_FORMAT;

    /* update the output size */
    AP4_LargeSize plaintext_size = 0;
    if (AP4_SUCCEEDED(decrypting_stream->GetSize(plaintext_size))) {
        self->output.size = plaintext_size;
    } else {
        self->output.size = self->input.encrypted_size;
    }
    
    /* create a reverse adapter */
    result = AP4_ByteStream_To_ATX_InputStream_Adapter_Create(decrypting_stream, &self->output.stream);
    decrypting_stream->Release();

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   DcfParserInput_SetStream
+---------------------------------------------------------------------*/
BLT_METHOD
DcfParserInput_SetStream(BLT_InputStreamUser* _self,
                         ATX_InputStream*     stream,
                         const BLT_MediaType* stream_media_type)
{
    DcfParser* self = ATX_SELF_M(input, DcfParser, BLT_InputStreamUser);
    BLT_Result result;
    
    /* check parameters and media type */
    if (stream == NULL            ||
        stream_media_type == NULL || 
       (stream_media_type->id != self->input.dcf1_media_type.id &&
        stream_media_type->id != self->input.dcf2_media_type.id)) {
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }

    /* if we had a stream, release it */
    ATX_RELEASE_OBJECT(self->output.stream);

    /* reset field values */
    self->input.encrypted_size  = 0;
    self->input.content_type[0] = 0;
    self->input.content_uri[0]  = 0;
    self->output.size           = 0;
    
    /* parse the stream header/info */
    if (stream_media_type->id == self->input.dcf1_media_type.id) {
        result = DcfParser_ParseV1Header(self, stream);
    } else {
        result = DcfParser_ParseV2Header(self, stream);
    }
    if (BLT_FAILED(result)) return result;

    /* lookup the media type id */
    BLT_Registry* registry = NULL;
    result = BLT_Core_GetRegistry(ATX_BASE(self, BLT_BaseMediaNode).core, &registry);
    if (BLT_FAILED(result)) return result;
    result = BLT_Registry_GetIdForName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        self->input.content_type,
        &self->output.media_type.id);
    if (BLT_FAILED(result)) {
        ATX_LOG_FINE_1("unregistered content type (%s)", self->input.content_type);
        return BLT_ERROR_UNSUPPORTED_CODEC;
    }
        
    /* update the stream info */
    BLT_StreamInfo stream_info;
    stream_info.size      = self->output.size;
    stream_info.data_type = self->input.content_type;
    stream_info.mask      = BLT_STREAM_INFO_MASK_SIZE | BLT_STREAM_INFO_MASK_DATA_TYPE;
    
    /* update the stream info */
    if (stream_info.mask && ATX_BASE(self, BLT_BaseMediaNode).context) {
        BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &stream_info);
    }

    return result;
}

/*----------------------------------------------------------------------
|   DcfParserInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
DcfParserInput_QueryMediaType(BLT_MediaPort*        _self,
                              BLT_Ordinal           index,
                              const BLT_MediaType** media_type)
{
    DcfParserInput* self = ATX_SELF(DcfParserInput, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = &self->dcf1_media_type;
        return BLT_SUCCESS;
    } else if (index == 1) {
        *media_type = &self->dcf2_media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(DcfParserInput)
    ATX_GET_INTERFACE_ACCEPT(DcfParserInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(DcfParserInput, BLT_InputStreamUser)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_InputStreamUser interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(DcfParserInput, BLT_InputStreamUser)
    DcfParserInput_SetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(DcfParserInput, 
                                         "input",
                                         STREAM_PULL,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(DcfParserInput, BLT_MediaPort)
    DcfParserInput_GetName,
    DcfParserInput_GetProtocol,
    DcfParserInput_GetDirection,
    DcfParserInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   DcfParserOutput_Construct
+---------------------------------------------------------------------*/
static void
DcfParserOutput_Construct(DcfParserOutput* /*self*/)
{
}

/*----------------------------------------------------------------------
|   DcfParserOutput_Destruct
+---------------------------------------------------------------------*/
static void
DcfParserOutput_Destruct(DcfParserOutput* /*self*/)
{
}

/*----------------------------------------------------------------------
|   DcfParserOutput_GetStream
+---------------------------------------------------------------------*/
BLT_METHOD
DcfParserOutput_GetStream(BLT_InputStreamProvider* _self,
                          ATX_InputStream**        stream)
{
    DcfParserOutput* self = ATX_SELF(DcfParserOutput, BLT_InputStreamProvider);

    // copy our stream pointer
    *stream = self->stream;
    if (self->stream == NULL) return BLT_ERROR_INVALID_MEDIA_FORMAT;

    // give a reference count to the caller
    ATX_REFERENCE_OBJECT(*stream);
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   DcfParserOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
DcfParserOutput_QueryMediaType(BLT_MediaPort*        _self,
                               BLT_Ordinal           index,
                               const BLT_MediaType** media_type)
{
    DcfParserOutput* self = ATX_SELF(DcfParserOutput, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = &self->media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(DcfParserOutput)
    ATX_GET_INTERFACE_ACCEPT(DcfParserOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(DcfParserOutput, BLT_InputStreamProvider)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(DcfParserOutput, 
                                         "output",
                                         STREAM_PULL,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(DcfParserOutput, BLT_MediaPort)
    DcfParserOutput_GetName,
    DcfParserOutput_GetProtocol,
    DcfParserOutput_GetDirection,
    DcfParserOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_InputStreamProvider interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(DcfParserOutput, BLT_InputStreamProvider)
    DcfParserOutput_GetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   DcfParser_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
DcfParser_Destroy(DcfParser* self)
{
    ATX_LOG_FINE("DcfParser::Destroy");

    /* destruct the members */
    DcfParserInput_Destruct(&self->input);
    DcfParserOutput_Destruct(&self->output);
    delete self->cipher_factory;

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    delete self;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    DcfParser_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
DcfParser_Deactivate(BLT_MediaNode* _self)
{
    DcfParser* self = ATX_SELF_EX(DcfParser, BLT_BaseMediaNode, BLT_MediaNode);

    ATX_LOG_FINER("DcfParser::Deactivate");

    /* release the stream */
    ATX_RELEASE_OBJECT(self->output.stream);

    /* call the base class method */
    BLT_BaseMediaNode_Deactivate(_self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   DcfParser_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
DcfParser_GetPortByName(BLT_MediaNode*  _self,
                        BLT_CString     name,
                        BLT_MediaPort** port)
{
    DcfParser* self = ATX_SELF_EX(DcfParser, BLT_BaseMediaNode, BLT_MediaNode);

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
|   DcfParser_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
DcfParser_Seek(BLT_MediaNode* _self,
               BLT_SeekMode*  mode,
               BLT_SeekPoint* point)
{
    DcfParser* self = ATX_SELF_EX(DcfParser, BLT_BaseMediaNode, BLT_MediaNode);

    /* estimate the seek point */
    if (ATX_BASE(self, BLT_BaseMediaNode).context == NULL) return BLT_FAILURE;
    BLT_Stream_EstimateSeekPoint(ATX_BASE(self, BLT_BaseMediaNode).context, *mode, point);
    if (!(point->mask & BLT_SEEK_POINT_MASK_TIME_STAMP)) {
        return BLT_FAILURE;
    }

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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(DcfParser)
    ATX_GET_INTERFACE_ACCEPT_EX(DcfParser, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(DcfParser, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(DcfParser, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    DcfParser_GetPortByName,
    BLT_BaseMediaNode_Activate,
    DcfParser_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    DcfParser_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(DcfParser, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   DcfParser_Construct
+---------------------------------------------------------------------*/
static void
DcfParser_Construct(DcfParser* self, BLT_Module* module, BLT_Core* core)
{
    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* construct the members */
    DcfParserInput_Construct(&self->input, module);
    DcfParserOutput_Construct(&self->output);
    
    /* look for a key manager */
    ATX_Properties* properties = NULL;
    if (BLT_SUCCEEDED(BLT_Core_GetProperties(core, &properties))) {
        ATX_PropertyValue value;
        if (ATX_SUCCEEDED(ATX_Properties_GetProperty(properties, 
                                                     BLT_KEY_MANAGER_PROPERTY, 
                                                     &value))) {
            if (value.type == ATX_PROPERTY_VALUE_TYPE_POINTER) {
                self->key_manager = (BLT_KeyManager*)value.data.pointer;
            }
        } else {
            ATX_LOG_FINE("no key manager");
        }

        /* check if we need to use a cipher factory */
        if (ATX_SUCCEEDED(ATX_Properties_GetProperty(properties, 
                                                     BLT_CIPHER_FACTORY_PROPERTY, 
                                                     &value))) {
            if (value.type == ATX_PROPERTY_VALUE_TYPE_POINTER) {
                self->cipher_factory = new BLT_Ap4CipherFactoryAdapter((BLT_CipherFactory*)value.data.pointer);
            }
        } else {
            ATX_LOG_FINE("no cipher factory");
        }
    }

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, DcfParser, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, DcfParser, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  DcfParserInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  DcfParserInput,  BLT_InputStreamUser);
    ATX_SET_INTERFACE(&self->output, DcfParserOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, DcfParserOutput, BLT_InputStreamProvider);
}

/*----------------------------------------------------------------------
|   DcfParser_Create
+---------------------------------------------------------------------*/
static BLT_Result
DcfParser_Create(BLT_Module*              module,
                 BLT_Core*                core, 
                 BLT_ModuleParametersType parameters_type,
                 BLT_AnyConst             parameters, 
                 BLT_MediaNode**          object)
{
    DcfParser* self;

    ATX_LOG_FINE("DcfParser::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate the object */
    self = new DcfParser();
    DcfParser_Construct(self, module, core);

    /* return the object */
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   DcfParserModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
DcfParserModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    DcfParserModule* self = ATX_SELF_EX(DcfParserModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*    registry;
    BLT_Result       result;

    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the ".dcf" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".dcf",
                                            "application/vnd.oma.drm.content");
    if (BLT_FAILED(result)) return result;

    /* register the ".odf" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".odf",
                                            "application/vnd.oma.drm.dcf");
    if (BLT_FAILED(result)) return result;

    /* register the ".oda" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".oda",
                                            "application/vnd.oma.drm.dcf");
    if (BLT_FAILED(result)) return result;

    /* register the ".odv" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".odv",
                                            "application/vnd.oma.drm.dcf");
    if (BLT_FAILED(result)) return result;

    /* get the type id for "application/vnd.oma.drm.content" */
    result = BLT_Registry_GetIdForName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "application/vnd.oma.drm.content",
        &self->dcf1_type_id);
    if (BLT_FAILED(result)) return result;
    ATX_LOG_FINE_1("DCF Parser Module::Attach (application/vnd.oma.drm.content type = %d)", self->dcf1_type_id);
    
    /* get the type id for "application/vnd.oma.drm.dcf" */
    result = BLT_Registry_GetIdForName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "application/vnd.oma.drm.dcf",
        &self->dcf2_type_id);
    if (BLT_FAILED(result)) return result;
    ATX_LOG_FINE_1("DCF Parser Module::Attach (application/vnd.oma.drm.dcf type = %d)", self->dcf2_type_id);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   DcfParserModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
DcfParserModule_Probe(BLT_Module*              _self, 
                      BLT_Core*                core,
                      BLT_ModuleParametersType parameters_type,
                      BLT_AnyConst             parameters,
                      BLT_Cardinal*            match)
{
    DcfParserModule* self = ATX_SELF_EX(DcfParserModule, BLT_BaseModule, BLT_Module);
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

            /* we need the input media type to be 'application/vnd.oma.drm.content' */
            /* or 'application/vnd.oma.drm.dcf'                                     */
            if (constructor->spec.input.media_type->id != self->dcf1_type_id &&
                constructor->spec.input.media_type->id != self->dcf2_type_id) {
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
                if (ATX_StringsEqual(constructor->name, "DcfParser")) {
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

            ATX_LOG_FINE_1("DcfParserModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(DcfParserModule)
    ATX_GET_INTERFACE_ACCEPT_EX(DcfParserModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(DcfParserModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(DcfParserModule, DcfParser)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(DcfParserModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    DcfParserModule_Attach,
    DcfParserModule_CreateInstance,
    DcfParserModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define DcfParserModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(DcfParserModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(DcfParserModule,
                                         "DCF Parser",
                                         "com.axiosys.parser.dcf",
                                         "1.1.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
