/*****************************************************************
|
|   BlueTune - HTTP Network Stream
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "Atomix.h"
#include "BltTypes.h"
#include "BltModule.h"
#include "BltHttpNetworkStream.h"
#include "BltNetworkStream.h"
#include "BltNetworkInputSource.h"
#include "BltStream.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.inputs.network.http")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const BLT_Size BLT_HTTP_NETWORK_STREAM_BUFFER_SIZE = 65536;

/*----------------------------------------------------------------------
|   HttpInputStream
+---------------------------------------------------------------------*/
// it is important to keep this structure a POD (no methods or object members)
// because the strict compilers will not like use using
// the offsetof() macro necessary when using ATX_SELF()
typedef struct {
    // interfaces
    ATX_IMPLEMENTS(ATX_InputStream);
    ATX_IMPLEMENTS(BLT_NetworkInputSource);
    ATX_IMPLEMENTS(ATX_Properties);
    ATX_IMPLEMENTS(ATX_Referenceable);

    // members
    ATX_Cardinal              m_ReferenceCount;
    NPT_HttpClient*           m_HttpClient;
    NPT_HttpUrl*              m_Url;
    NPT_HttpResponse*         m_Response;
    NPT_InputStreamReference* m_InputStream;
    NPT_LargeSize             m_ContentLength;
    bool                      m_Eos;
    bool                      m_IsIcy;
    bool                      m_CanSeek;
    unsigned int              m_IcyMetaInterval;
    unsigned int              m_IcyMetaCounter;
    BLT_Stream*               m_Context;
} HttpInputStream;

/*----------------------------------------------------------------------
|   HttpInputStream_MapResult
+---------------------------------------------------------------------*/
static BLT_Result
HttpInputStream_MapResult(NPT_Result result)
{
    switch (result) {
        case NPT_ERROR_EOS: return ATX_ERROR_EOS;
        default: return result;
    }
}

/*----------------------------------------------------------------------
|   HttpInputStream_GetProperty
+---------------------------------------------------------------------*/
ATX_METHOD
HttpInputStream_GetProperty(ATX_Properties*    _self, 
                            const char*        name,
                            ATX_PropertyValue* value)
{
    HttpInputStream* self = ATX_SELF(HttpInputStream, ATX_Properties);
    
    if (name != NULL && ATX_StringsEqual(name, ATX_INPUT_STREAM_PROPERTY_SEEK_SPEED)) {
        value->type = ATX_PROPERTY_VALUE_TYPE_INTEGER;
        if (self->m_CanSeek) {
            value->data.integer = ATX_INPUT_STREAM_SEEK_SPEED_SLOW;
        } else {
            value->data.integer = ATX_INPUT_STREAM_SEEK_SPEED_NO_SEEK;
        }
        return ATX_SUCCESS;
    }
    
    return ATX_ERROR_NO_SUCH_PROPERTY;
}

/*----------------------------------------------------------------------
|   HttpInputStream_GetMediaType
+---------------------------------------------------------------------*/
static BLT_Result
HttpInputStream_GetMediaType(HttpInputStream*  self,
                             BLT_Core*         core, 
                             BLT_MediaType**   media_type)
{
    BLT_Registry* registry;
    BLT_UInt32    type_id;
    BLT_Result    result;

    /* check that we have what we need */
    if (self->m_Response == NULL || self->m_Response->GetEntity() == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    // query the registry for the content type
    NPT_String content_type = self->m_Response->GetEntity()->GetContentType();
    ATX_LOG_FINE_1("HttpInputStream::GetMediaType - Content-Type = %s", 
                   content_type.GetChars());
    if (content_type.GetLength() == 0 && self->m_IsIcy) {
        // if the content type is not specified, and this is an ICY stream,
        // assume MP3
        content_type = "audio/mpeg";
    }
    // remove trailing parameters of the content type
    int separator = content_type.Find(';');
    if (separator >= 0) {
        content_type.SetLength(separator);
    }
    result = BLT_Registry_GetIdForName(registry, 
                                       BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS, 
                                       content_type.GetChars(), 
                                       &type_id);
    if (BLT_FAILED(result)) {
        // try to guess based on the name extension
        if (self->m_Url) {
            int dot = self->m_Url->GetPath().ReverseFind('.');
            if (dot >= 0) {
                // query the registry
                const char* extension = self->m_Url->GetPath().GetChars()+dot;
                result = BLT_Registry_GetMediaTypeIdForExtension(registry, 
                                                                 extension, 
                                                                 &type_id);
                if (BLT_FAILED(result)) return BLT_FAILURE;
            }
        }

        return result;
    }

    /* create the media type */
    BLT_MediaType_Clone(&BLT_MediaType_Unknown, media_type);
    (*media_type)->id = type_id;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   HttpInputStream_Destroy
+---------------------------------------------------------------------*/
static void
HttpInputStream_Destroy(HttpInputStream* self)
{
    delete self->m_HttpClient;
    delete self->m_Url;
    delete self->m_Response;
    delete self->m_InputStream;
    delete self;
}

/*----------------------------------------------------------------------
|   HttpInputStream_SendRequest
+---------------------------------------------------------------------*/
BLT_METHOD
HttpInputStream_SendRequest(HttpInputStream* self, NPT_Position position)
{
    // send the request
    NPT_Result      result = BLT_FAILURE;
    NPT_HttpRequest request(*self->m_Url, NPT_HTTP_METHOD_GET);

    // delete any previous response we may have
    delete self->m_Response;
    self->m_Response = NULL;
    *self->m_InputStream = NULL;

    // handle a non-zero start position
    if (position) {
        if (self->m_ContentLength == position) {
            // special case: seek to end of stream
            self->m_Eos = true;
            return BLT_SUCCESS;
        }
        NPT_String range = "bytes="+NPT_String::FromInteger(position);
        range += "-";
        request.GetHeaders().SetHeader(NPT_HTTP_HEADER_RANGE, range);
        ATX_LOG_FINE_1("HttpInputStream_SendRequest - seek, %s", range.GetChars());
    }

    // add the ICY header that says we can deal with ICY metadata
    request.GetHeaders().SetHeader("icy-metadata", "1");

    // send the request
    result = self->m_HttpClient->SendRequest(request, self->m_Response);
    if (NPT_FAILED(result)) return result;

    switch (self->m_Response->GetStatusCode()) {
        case 200:
            // if this is a Range request, expect a 206 instead
            if (position) result = BLT_ERROR_PROTOCOL_FAILURE;
            
            // get the body stream and size
            self->m_Response->GetEntity()->GetInputStream(*self->m_InputStream);
            self->m_ContentLength = self->m_Response->GetEntity()->GetContentLength();
            result = BLT_SUCCESS;
            break;

        case 206:
            // if this is not a Range request, expect a 200 instead
            if (position == 0) result = BLT_ERROR_PROTOCOL_FAILURE;
            
            // get the body stream
            self->m_Response->GetEntity()->GetInputStream(*self->m_InputStream);
            result = BLT_SUCCESS;
            break;

        case 401:
        case 403:
            result = ATX_ERROR_ACCESS_DENIED;
            break;
            
        case 404:
            result = BLT_ERROR_STREAM_INPUT_NOT_FOUND;
            break;

        default:
            result = BLT_FAILURE;
    }

    // see if we can seek 
    self->m_CanSeek = false;
    const NPT_String* accept_range = self->m_Response->GetHeaders().GetHeaderValue("Accept-Ranges");
    if (accept_range) {
        if (*accept_range == "bytes") {
            ATX_LOG_FINE("HttpInputStream::SendRequest - stream is seekable");
            self->m_CanSeek = true;
        }
    }
    
    // see if this is an ICY response
    self->m_IcyMetaCounter = 0;
    if (self->m_Response->GetProtocol() == "ICY") {
        self->m_IsIcy   = true;
    } else {
        self->m_IsIcy = false;
    }
    const NPT_String* icy_metaint = self->m_Response->GetHeaders().GetHeaderValue("icy-metaint");
    if (icy_metaint) {
        unsigned int interval = 0;
        icy_metaint->ToInteger(interval, ATX_TRUE);
        self->m_IcyMetaInterval = interval;
        self->m_IsIcy = true;
    } else {
        self->m_IcyMetaInterval = 0;
    }

    return result;
}

/*----------------------------------------------------------------------
|   HttpInputStream_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
HttpInputStream_Attach(BLT_NetworkInputSource* _self,
                       BLT_Stream*             stream)
{
    HttpInputStream* self = ATX_SELF(HttpInputStream, BLT_NetworkInputSource);
    self->m_Context = stream;

    // read the http headers
    ATX_Properties* properties;
    if (stream && BLT_SUCCEEDED(BLT_Stream_GetProperties(stream, &properties))) {
        for (NPT_List<NPT_HttpHeader*>::Iterator i = self->m_Response->GetHeaders().GetHeaders().GetFirstItem();
             i;
             ++i) {
            NPT_HttpHeader* header = *i;
            ATX_PropertyValue value;
            value.type = ATX_PROPERTY_VALUE_TYPE_STRING;
            if (header->GetName().StartsWith("icy-", true)) {
                NPT_String name = "ICY/";
                name += header->GetName().GetChars()+4;
                value.data.string = header->GetValue();
                ATX_Properties_SetProperty(properties, name, &value);
            }

            if (header->GetName().Compare("icy-name", true) == 0) {
                value.data.string = header->GetValue();
                ATX_Properties_SetProperty(properties, "Tags/RadioName", &value);
            } else if (header->GetName().Compare("icy-genre", true) == 0) {
                value.data.string = header->GetValue();
                ATX_Properties_SetProperty(properties, "Tags/Genre", &value);
            }
        }
    }

    // set some optional stream info flags
    if (self->m_IsIcy || self->m_ContentLength==0) {
        // consider that this stream is continuous (not a discrete file)
        BLT_StreamInfo info;
        BLT_Stream_GetInfo(stream, &info);
        info.flags |= BLT_STREAM_INFO_FLAG_CONTINUOUS; 
        info.mask   = BLT_STREAM_INFO_MASK_FLAGS;
        BLT_Stream_SetInfo(stream, &info);
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   HttpInputStream_Detach
+---------------------------------------------------------------------*/
BLT_METHOD
HttpInputStream_Detach(BLT_NetworkInputSource* _self)
{
    HttpInputStream* self = ATX_SELF(HttpInputStream, BLT_NetworkInputSource);
    self->m_Context = NULL;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   HttpInputStream_Read
+---------------------------------------------------------------------*/
BLT_METHOD
HttpInputStream_Read(ATX_InputStream* _self,
                     ATX_Any          buffer,
                     ATX_Size         bytes_to_read,
                     ATX_Size*        bytes_read)
{
    HttpInputStream* self = ATX_SELF(HttpInputStream, ATX_InputStream);
    if (self->m_Eos) return ATX_ERROR_EOS;
    if (self->m_InputStream->IsNull()) return ATX_ERROR_INVALID_STATE;

    // see if we need to truncate the read because of ICY metadata
    if (self->m_IsIcy && self->m_IcyMetaInterval) {
        unsigned int can_read = self->m_IcyMetaInterval-self->m_IcyMetaCounter;
        if (can_read < bytes_to_read) bytes_to_read = can_read;
    }

    // read and count
    NPT_Size local_bytes_read;
    NPT_Result result = (*(self->m_InputStream))->Read(buffer, bytes_to_read, &local_bytes_read);
    if (NPT_SUCCEEDED(result)) {
        if (bytes_read) *bytes_read = local_bytes_read;
        self->m_IcyMetaCounter += local_bytes_read;
    }

    // see if we should read ICY metadata now 
    if (self->m_IsIcy && self->m_IcyMetaInterval && 
        self->m_IcyMetaCounter == self->m_IcyMetaInterval) {
        unsigned char meta_size = 0;
        result = (*(self->m_InputStream))->Read(&meta_size, 1);
        if (NPT_SUCCEEDED(result) && meta_size != 0) {
            char* meta_value = new char[meta_size*16+1];
            meta_value[meta_size*16] = '\0'; // terminate
            result = (*(self->m_InputStream))->ReadFully(meta_value, meta_size*16);
            if (NPT_SUCCEEDED(result)) {
                // extract the title
                NPT_String title(meta_value);
                int quote_1 = title.Find("StringTitle='");
                if (quote_1) {
                    quote_1 += 13;
                    int quote_2 = title.Find("';", quote_1);
                    if (quote_2) {
                        title.SetLength(quote_2);

                        // notify of the title
                        ATX_Properties* properties;
                        if (self->m_Context && BLT_SUCCEEDED(BLT_Stream_GetProperties(self->m_Context, &properties))) {
                            ATX_PropertyValue pvalue;
                            pvalue.type = ATX_PROPERTY_VALUE_TYPE_STRING;
                            pvalue.data.string = title.GetChars()+quote_1+1;
                            ATX_Properties_SetProperty(properties, "Tags/Title", &pvalue);
                        }
                    }
                }
            }
            delete[] meta_value;
       }

       // reset the counter
       self->m_IcyMetaCounter = 0;
    }

    return HttpInputStream_MapResult(result);
}

/*----------------------------------------------------------------------
|   HttpInputStream_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
HttpInputStream_Seek(ATX_InputStream* _self, 
                     ATX_Position     where)
{
    HttpInputStream* self = ATX_SELF(HttpInputStream, ATX_InputStream);

    // special case to detect if we seek to the very end of the stream
    if (where != 0 && where == self->m_ContentLength) {
        self->m_Eos = true;
        return NPT_SUCCESS;
    }
    
    // check if we can seek
    if ((!self->m_CanSeek) || (self->m_ContentLength == 0)) {
        return BLT_ERROR_NOT_SUPPORTED;
    }
    
    // seek by emitting a new request with a range
    NPT_Result result = HttpInputStream_SendRequest(self, where);
    if (NPT_SUCCEEDED(result)) self->m_Eos = false;
    return result;
}

/*----------------------------------------------------------------------
|   HttpInputStream_Tell
+---------------------------------------------------------------------*/
BLT_METHOD
HttpInputStream_Tell(ATX_InputStream* _self, 
                     ATX_Position*    position)
{
    HttpInputStream* self = ATX_SELF(HttpInputStream, ATX_InputStream);
    if (self->m_Eos) {
        if (self->m_Response) {
            *position = self->m_Response->GetEntity()->GetContentLength();
        }
    }
    if (self->m_InputStream->IsNull()) return ATX_ERROR_INVALID_STATE;
    NPT_Position _position;
    ATX_Result result = HttpInputStream_MapResult((*(self->m_InputStream))->Tell(_position));
    if (position) *position = _position;
    return result;
}

/*----------------------------------------------------------------------
|   HttpInputStream_GetSize
+---------------------------------------------------------------------*/
BLT_METHOD
HttpInputStream_GetSize(ATX_InputStream* _self, 
                        ATX_LargeSize*   size)
{
    HttpInputStream* self = ATX_SELF(HttpInputStream, ATX_InputStream);
    *size = self->m_ContentLength;
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   HttpInputStream_GetAvailable
+---------------------------------------------------------------------*/
BLT_METHOD
HttpInputStream_GetAvailable(ATX_InputStream* _self, 
                             ATX_LargeSize*   available)
{
    HttpInputStream* self = ATX_SELF(HttpInputStream, ATX_InputStream);
    *available = 0;
    if (self->m_InputStream->IsNull()) return ATX_ERROR_INVALID_STATE;
    NPT_LargeSize _available;
    ATX_Result result = HttpInputStream_MapResult((*(self->m_InputStream))->GetAvailable(_available));
    if (available) *available = _available;
    return result;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(HttpInputStream)
    ATX_GET_INTERFACE_ACCEPT(HttpInputStream, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT(HttpInputStream, ATX_Properties)
    ATX_GET_INTERFACE_ACCEPT(HttpInputStream, ATX_InputStream)
    ATX_GET_INTERFACE_ACCEPT(HttpInputStream, BLT_NetworkInputSource)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    ATX_InputStream interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(HttpInputStream, ATX_InputStream)
    HttpInputStream_Read,
    HttpInputStream_Seek,
    HttpInputStream_Tell,
    HttpInputStream_GetSize,
    HttpInputStream_GetAvailable
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Properties interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_STATIC_PROPERTIES_INTERFACE(HttpInputStream)

/*----------------------------------------------------------------------
|    BLT_NetworkInputSource interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(HttpInputStream, BLT_NetworkInputSource)
    HttpInputStream_Attach,
    HttpInputStream_Detach
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE(HttpInputStream, m_ReferenceCount)

/*----------------------------------------------------------------------
|   HttpInputStream_Create
+---------------------------------------------------------------------*/
static HttpInputStream*
HttpInputStream_Create(const char* url)
{
    // create and initialize
    HttpInputStream* stream   = new HttpInputStream;
    stream->m_ReferenceCount  = 1;
    stream->m_HttpClient      = new NPT_HttpClient;
    stream->m_Url             = new NPT_HttpUrl(url);
    stream->m_Response        = NULL;
    stream->m_InputStream     = new NPT_InputStreamReference;
    stream->m_ContentLength   = 0;
    stream->m_Eos             = false;
    stream->m_IsIcy           = false;
    stream->m_CanSeek         = false;
    stream->m_IcyMetaInterval = 0;
    stream->m_IcyMetaCounter  = 0;
    stream->m_Context         = NULL;

    // setup interfaces
    ATX_SET_INTERFACE(stream, HttpInputStream, ATX_InputStream);
    ATX_SET_INTERFACE(stream, HttpInputStream, ATX_Properties);
    ATX_SET_INTERFACE(stream, HttpInputStream, BLT_NetworkInputSource);
    ATX_SET_INTERFACE(stream, HttpInputStream, ATX_Referenceable);

    return stream;
}

/*----------------------------------------------------------------------
|   BLT_HttpNetworkStream_Create
+---------------------------------------------------------------------*/
BLT_Result 
BLT_HttpNetworkStream_Create(const char*              url, 
                             BLT_Core*                core,
                             ATX_InputStream**        stream,
                             BLT_NetworkInputSource** source,
                             BLT_MediaType**          media_type)
{
    BLT_Result result = BLT_FAILURE;

    // default return value
    *stream = NULL;
    *source = NULL;
    *media_type = NULL;

    // create a stream object
    HttpInputStream* http_stream = HttpInputStream_Create(url);
    if (!http_stream->m_Url->IsValid()) {
        HttpInputStream_Destroy(http_stream);
        return BLT_ERROR_INVALID_PARAMETERS;
    }
    *source = &ATX_BASE(http_stream, BLT_NetworkInputSource);

    // send the request
    result = HttpInputStream_SendRequest(http_stream, 0);
    if (NPT_FAILED(result)) return result;

    // see if we can determine the media type
    HttpInputStream_GetMediaType(http_stream, core, media_type);

    // create a stream adapter
    ATX_InputStream* adapted_input_stream = &ATX_BASE(http_stream, ATX_InputStream);
    BLT_NetworkStream_Create(BLT_HTTP_NETWORK_STREAM_BUFFER_SIZE, adapted_input_stream, stream);

    return BLT_SUCCESS;
}

