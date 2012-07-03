/*****************************************************************
|
|   Atomix - HTTP
|
| Copyright (c) 2002-2010, Axiomatic Systems, LLC.
| All rights reserved.
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions are met:
|     * Redistributions of source code must retain the above copyright
|       notice, this list of conditions and the following disclaimer.
|     * Redistributions in binary form must reproduce the above copyright
|       notice, this list of conditions and the following disclaimer in the
|       documentation and/or other materials provided with the distribution.
|     * Neither the name of Axiomatic Systems nor the
|       names of its contributors may be used to endorse or promote products
|       derived from this software without specific prior written permission.
|
| THIS SOFTWARE IS PROVIDED BY AXIOMATIC SYSTEMS ''AS IS'' AND ANY
| EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
| WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
| DISCLAIMED. IN NO EVENT SHALL AXIOMATIC SYSTEMS BE LIABLE FOR ANY
| DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
| (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
| ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
| SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
|
 ****************************************************************/

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "AtxTypes.h"
#include "AtxDefs.h"
#include "AtxResults.h"
#include "AtxUtils.h"
#include "AtxInterfaces.h"
#include "AtxReferenceable.h"
#include "AtxDestroyable.h"
#include "AtxList.h"
#include "AtxHttp.h"
#include "AtxSockets.h"
#include "AtxVersion.h"
#include "AtxLogging.h"

/*----------------------------------------------------------------------
|    logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("atomix.http")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
struct ATX_HttpClient {
    struct {
        ATX_Boolean follow_redirect;
    } options;
};

typedef struct {
    ATX_String username;
    ATX_String password;
    ATX_String host;
    int        port;
    ATX_String path;
} ATX_HttpUrl;

typedef struct {
    ATX_String name;
    ATX_String value;
} ATX_HttpHeader;

struct ATX_HttpMessage {
    ATX_String       protocol;
    ATX_List*        headers;
    ATX_InputStream* body;
};

struct ATX_HttpRequest {
    ATX_HttpMessage base;
    ATX_HttpUrl     url;
    ATX_String      method;
};

struct ATX_HttpResponse {
    ATX_HttpMessage base;
    ATX_UInt32      status_code;
    ATX_String      reason_phrase;
};

typedef enum {
    ATX_HTTP_URL_PARSER_STATE_START,
    ATX_HTTP_URL_PARSER_STATE_LEADING_SLASH,
    ATX_HTTP_URL_PARSER_STATE_HOST,
    ATX_HTTP_URL_PARSER_STATE_PORT
} ATX_HttpUrl_ParserState;

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define ATX_HTTP_DEFAULT_PROTOCOL "HTTP/1.0"
#define ATX_HTTP_MAX_LINE_SIZE    2048
#define ATX_HTTP_DEFAULT_PORT     80
#define ATX_HTTP_INVALID_PORT     (-1)

#define ATX_HTTP_RESOLVER_TIMEOUT 10000 /* 10 seconds */
#define ATX_HTTP_CONNECT_TIMEOUT  15000 /* 15 seconds */
#define ATX_HTTP_MAX_REDIRECTS    20

#define ATX_HTTP_HEADER_CONTENT_LENGTH      "Content-Length"
#define ATX_HTTP_HEADER_HOST                "Host"
#define ATX_HTTP_HEADER_CONNECTION          "Connection"
#define ATX_HTTP_HEADER_USER_AGENT          "User-Agent"
#define ATX_HTTP_HEADER_CONTENT_LENGTH      "Content-Length"
#define ATX_HTTP_HEADER_CONTENT_TYPE        "Content-Type"
#define ATX_HTTP_HEADER_CONTENT_ENCODING    "Content-Encoding"
#define ATX_HTTP_HEADER_LOCATION            "Location"

#define ATX_HTTP_HEADER_DEFAULT_AGENT "Atomix/"ATX_ATOMIX_VERSION_STRING 

/*----------------------------------------------------------------------
|    ATX_Http_NumToAscii
+---------------------------------------------------------------------*/
static void
ATX_Http_NumToAscii(char* string, unsigned long num)
{
    char*         current;
    int           char_count = 0;
    unsigned long n = num;
    do {
        char_count++;
        n /= 10;
    } while (n);
    current = &string[n];
    *current-- = '\0';
    n = num;
    do {
        *current++ = '0' + (unsigned char)(n%10);
        n /= 10;
    } while(n);
}

/*----------------------------------------------------------------------
|    ATX_Http_FindChar
+---------------------------------------------------------------------*/
static const char*
ATX_Http_FindChar(const char* string, char c)
{
    while (*string != 0) {
        if (*string == c) return string;
        string++;
    }

    /* not found */
    return NULL;
}

/*----------------------------------------------------------------------
|    ATX_Http_SkipWhitespace
+---------------------------------------------------------------------*/
static const char*
ATX_Http_SkipWhitespace(const char* string)
{
    while (*string == ' ' || *string == '\t') {
        string++;
    }
    return string;
}

/*----------------------------------------------------------------------
|   ATX_HttpUrl_Construct
+---------------------------------------------------------------------*/
static ATX_Result
ATX_HttpUrl_Construct(ATX_HttpUrl* self, const char* url)
{
    ATX_HttpUrl_ParserState state;
    const char*             mark;
    char                    c;

    /* set default values */
    ATX_INIT_STRING(self->username);
    ATX_INIT_STRING(self->password);
    ATX_INIT_STRING(self->host);
    self->port     = ATX_HTTP_DEFAULT_PORT;
    self->path     = ATX_String_Create("/");

    /* check parameters */
    if (url == NULL) return ATX_ERROR_INVALID_PARAMETERS;

    /* check that this is an http url */
    if ((url[0] != 'h' && url[0] != 'H') ||
        (url[1] != 't' && url[1] != 'T') ||
        (url[2] != 't' && url[2] != 'T') ||
        (url[3] != 'p' && url[3] != 'P') ||
        (url[4] != ':')) {
            return ATX_ERROR_INVALID_SYNTAX;
        }

        /* intialize the parser */
        state = ATX_HTTP_URL_PARSER_STATE_START;
        url+= 5;
        mark = url;

        /* parse the URL */
        do  {
            c = *url++;
            switch (state) {
case ATX_HTTP_URL_PARSER_STATE_START:
    if (c == '/') {
        state = ATX_HTTP_URL_PARSER_STATE_LEADING_SLASH;
    } else {
        return ATX_ERROR_INVALID_SYNTAX;
    }
    break;

case ATX_HTTP_URL_PARSER_STATE_LEADING_SLASH:
    if (c == '/') {
        state = ATX_HTTP_URL_PARSER_STATE_HOST;
        mark = url;
    } else {
        return ATX_ERROR_INVALID_SYNTAX;
    }
    break;

case ATX_HTTP_URL_PARSER_STATE_HOST:
    if (c == ':') {
        ATX_String_AssignN(&self->host, mark, (ATX_Size)(url-mark));
        state = ATX_HTTP_URL_PARSER_STATE_PORT;
    } else if (c == '/' || c == 0) {
        ATX_String_AssignN(&self->host, mark, (ATX_Size)(url-1-mark));
        self->port = ATX_HTTP_DEFAULT_PORT;
        if (c == '/') ATX_String_Append(&self->path, url);
        return ATX_SUCCESS;
    }
    break;

case ATX_HTTP_URL_PARSER_STATE_PORT:
    if (c >= '0' && c <= '9') {
        self->port = self->port*10+(c-'0');
    } else if (c == '/' || c == 0) {
        if (c == '/') ATX_String_Append(&self->path, url);
        return ATX_SUCCESS;
    } else {
        /* invalid character */
        self->port = ATX_HTTP_INVALID_PORT;
        return ATX_ERROR_INVALID_SYNTAX;
    }
            }
        } while (c);

        return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_HttpUrl_Destruct
+---------------------------------------------------------------------*/
static ATX_Result
ATX_HttpUrl_Destruct(ATX_HttpUrl* self)
{
    ATX_String_Destruct(&self->username);
    ATX_String_Destruct(&self->password);
    ATX_String_Destruct(&self->host);
    ATX_String_Destruct(&self->path);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_HttpClient_Create
+---------------------------------------------------------------------*/
ATX_Result
ATX_HttpClient_Create(ATX_HttpClient** client)
{
    /* allocate memory */
    *client = (ATX_HttpClient*)ATX_AllocateZeroMemory(sizeof(ATX_HttpClient));

    /* construct the object */
    (*client)->options.follow_redirect = ATX_TRUE;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_HttpClient_Destroy
+---------------------------------------------------------------------*/
ATX_Result
ATX_HttpClient_Destroy(ATX_HttpClient* self)
{
    /* destruct the object */

    /* free the memory */
    ATX_FreeMemory((void*)self);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_HttpClient_SetOptionBool
+---------------------------------------------------------------------*/
ATX_Result
ATX_HttpClient_SetOptionBool(ATX_HttpClient* self,
                             ATX_CString     option,
                             ATX_Boolean     value)
{
    if (ATX_StringsEqual(option, ATX_HTTP_CLIENT_OPTION_FOLLOW_REDIRECT)) {
        self->options.follow_redirect = value;
    } else {
        return ATX_ERROR_NO_SUCH_ITEM;
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_HttpClient_SendRequestOnce
+---------------------------------------------------------------------*/
static ATX_Result
ATX_HttpClient_SendRequestOnce(ATX_HttpClient*    self, 
                               ATX_HttpRequest*   request, 
                               ATX_HttpResponse** response)

{
    ATX_SocketAddress address;
    ATX_Socket*       connection = NULL;
    ATX_InputStream*  input_stream = NULL;
    ATX_OutputStream* output_stream = NULL;
    ATX_Result        result;

    ATX_COMPILER_UNUSED(self);

    /* set default return value */
    *response = NULL;

    /* resolve the host address */
    ATX_LOG_INFO_1("ATX_HttpClient::SendRequest - resolving name [%s]...",
                   ATX_CSTR(request->url.host));
    result = ATX_IpAddress_ResolveName(&address.ip_address, 
                                       ATX_CSTR(request->url.host),
                                       ATX_HTTP_RESOLVER_TIMEOUT);
    if (ATX_FAILED(result)) return result;
    address.port = request->url.port;
    ATX_LOG_INFO("ATX_HttpClient::SendRequest - name resolved");

    /* setup some headers */
    ATX_HttpMessage_SetHeader((ATX_HttpMessage*)request,
                              ATX_HTTP_HEADER_CONNECTION,
                              "close");
    ATX_HttpMessage_SetHeader((ATX_HttpMessage*)request,
                              ATX_HTTP_HEADER_HOST,
                              ATX_CSTR(request->url.host));
    ATX_HttpMessage_SetHeader((ATX_HttpMessage*)request,
                              ATX_HTTP_HEADER_USER_AGENT,
                              ATX_HTTP_HEADER_DEFAULT_AGENT);

    /* create a socket to connect to the server */
    result = ATX_TcpClientSocket_Create(&connection);
    if (ATX_FAILED(result)) return result;

    /* connect to the server */
    ATX_LOG_INFO_1("ATX_HttpClient::SendRequest - connecting on port %d...",
               request->url.port);
    result = ATX_Socket_Connect(connection, &address, ATX_HTTP_CONNECT_TIMEOUT);
    if (ATX_FAILED(result)) goto end;

    /* emit the request onto the connection */
    result = ATX_Socket_GetOutputStream(connection, &output_stream);
    if (ATX_FAILED(result)) goto end;
    result = ATX_HttpRequest_Emit(request, output_stream);
    if (ATX_FAILED(result)) goto end;

    /* create a response from the connection's input stream */
    result = ATX_Socket_GetInputStream(connection, &input_stream);
    if (ATX_FAILED(result)) goto end;
    result = ATX_HttpResponse_CreateFromStream(input_stream, response);
    if (ATX_FAILED(result)) {
        *response = NULL;
        goto end;
    }

end:
    if (ATX_FAILED(result)) {
        if (*response != NULL) {
            ATX_HttpResponse_Destroy(*response);
        }
    }
    ATX_RELEASE_OBJECT(input_stream);
    ATX_RELEASE_OBJECT(output_stream);
    ATX_DESTROY_OBJECT(connection);
    return result;
}

/*----------------------------------------------------------------------
|   ATX_HttpClient_SendRequest
+---------------------------------------------------------------------*/
ATX_Result
ATX_HttpClient_SendRequest(ATX_HttpClient*    self, 
                           ATX_HttpRequest*   request, 
                           ATX_HttpResponse** response)
{
    ATX_Cardinal watchdog = ATX_HTTP_MAX_REDIRECTS;
    ATX_Boolean  keep_going;
    ATX_Result   result;

    do {
        keep_going = ATX_FALSE;
        result = ATX_HttpClient_SendRequestOnce(self, request, response);
        if (ATX_FAILED(result)) break;
        if (*response && self->options.follow_redirect &&
            (ATX_String_Equals(&request->method, ATX_HTTP_METHOD_GET, ATX_FALSE) ||
             ATX_String_Equals(&request->method, ATX_HTTP_METHOD_HEAD, ATX_FALSE)) &&
            ((*response)->status_code == 301 ||
             (*response)->status_code == 302 ||
             (*response)->status_code == 303 ||
             (*response)->status_code == 307)) {
            /* handle redirect */
            const ATX_String* location = 
                ATX_HttpMessage_GetHeader((ATX_HttpMessage*)*response,
                                          ATX_HTTP_HEADER_LOCATION);
            if (location) {
                /* replace the request url */
                ATX_HttpUrl url;
                result = ATX_HttpUrl_Construct(&url, ATX_String_GetChars(location));
                if (ATX_SUCCEEDED(result)) {
                    ATX_LOG_FINE_1("ATX_HttpClient::SendRequest - redirecting to %s",
                                   ATX_String_GetChars(location));
                    ATX_HttpUrl_Destruct(&request->url);
                    request->url = url;
                    keep_going = ATX_TRUE;
                    ATX_HttpResponse_Destroy(*response);
                    *response = NULL;
                } else {
                    ATX_LOG_FINE_1("ATX_HttpClient::SendRequest - failed to create redirection URL (%d)", result);
                    break;
                }
            }
        }       
    } while (keep_going && watchdog--);

    return result;
}

/*----------------------------------------------------------------------
|    ATX_HttpHeader_Create
+---------------------------------------------------------------------*/
static ATX_Result 
ATX_HttpHeader_Create(ATX_CString      name, 
                      ATX_CString      value,
                      ATX_HttpHeader** header)
{
    /* allocate a new object */
    *header = (ATX_HttpHeader*)ATX_AllocateZeroMemory(sizeof(ATX_HttpHeader));
    if (*header == NULL) {
        return ATX_ERROR_OUT_OF_MEMORY;
    }

    /* construct the object */
    (*header)->name  = ATX_String_Create(name);
    (*header)->value = ATX_String_Create(value);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_HttpHeader_Destroy
+---------------------------------------------------------------------*/
static ATX_Result 
ATX_HttpHeader_Destroy(ATX_HttpHeader* header)
{
    /* free the strings */
    ATX_String_Destruct(&header->name);
    ATX_String_Destruct(&header->value);

    /* free the object */
    ATX_FreeMemory(header);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_HttpHeader_SetValue
+---------------------------------------------------------------------*/
static ATX_Result 
ATX_HttpHeader_SetValue(ATX_HttpHeader* header, ATX_CString value)
{
    /* copy the new value */
    return ATX_String_Assign(&header->value, value);
}

/*----------------------------------------------------------------------
|    ATX_HttpMessage_Construct
+---------------------------------------------------------------------*/
static ATX_Result 
ATX_HttpMessage_Construct(ATX_HttpMessage* message)
{
    ATX_Result result;

    /* construct the object */
    result = ATX_List_Create(&message->headers);
    if (ATX_FAILED(result)) return result;

    /* set the default protocol */
    message->protocol = ATX_String_Create(ATX_HTTP_DEFAULT_PROTOCOL);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_HttpMessage_Create
+---------------------------------------------------------------------*/
ATX_Result 
ATX_HttpMessage_Create(ATX_HttpMessage** message)
{
    ATX_Result result;

    /* allocate a new object */
    *message = (ATX_HttpMessage*)ATX_AllocateZeroMemory(sizeof(ATX_HttpMessage));
    if (*message == NULL) {
        return ATX_ERROR_OUT_OF_MEMORY;
    }

    /* construct the object */
    result = ATX_HttpMessage_Construct(*message);
    if (ATX_FAILED(result)) {
        ATX_FreeMemory(*message);
        return result;
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_HttpMessage_Destruct
+---------------------------------------------------------------------*/
static ATX_Result
ATX_HttpMessage_Destruct(ATX_HttpMessage* message)
{
    /* destroy all headers */
    ATX_ListItem* item = ATX_List_GetFirstItem(message->headers);
    while (item) {
        ATX_HttpHeader_Destroy((ATX_HttpHeader*)ATX_ListItem_GetData(item));
        item = ATX_ListItem_GetNext(item);
    }
    ATX_List_Destroy(message->headers);

    /* free the protocol string */
    ATX_String_Destruct(&message->protocol);

    /* release the body stream */
    ATX_RELEASE_OBJECT(message->body);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_HttpMessage_Destroy
+---------------------------------------------------------------------*/
ATX_Result
ATX_HttpMessage_Destroy(ATX_HttpMessage* message)
{
    /* destruct the object */
    ATX_HttpMessage_Destruct(message);

    /* free the object */
    ATX_FreeMemory(message);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_HttpMessage_SetHeader
+---------------------------------------------------------------------*/
ATX_Result
ATX_HttpMessage_SetHeader(ATX_HttpMessage* message,
                          ATX_CString      name, 
                          ATX_CString      value)
{
    ATX_HttpHeader* header;
    ATX_Result      result;

    /* find if the header already exists */
    ATX_ListItem* item = ATX_List_GetFirstItem(message->headers);
    while (item) {
        header = (ATX_HttpHeader*)ATX_ListItem_GetData(item);
        if (ATX_String_Equals(&header->name, name, ATX_TRUE)) {
            /* found a match */
            return ATX_HttpHeader_SetValue(header, value);
        }
        item = ATX_ListItem_GetNext(item);
    }

    /* create a new header */
    result = ATX_HttpHeader_Create(name, value, &header);
    if (ATX_FAILED(result)) return result;
    return ATX_List_AddData(message->headers, header);
}

/*----------------------------------------------------------------------
|    ATX_HttpMessage_GetHeader
+---------------------------------------------------------------------*/
const ATX_String*
ATX_HttpMessage_GetHeader(const ATX_HttpMessage* message, ATX_CString name)
{
    /* find the header */
    ATX_ListItem* item = ATX_List_GetFirstItem(message->headers);
    while (item) {
        ATX_HttpHeader* header = (ATX_HttpHeader*)ATX_ListItem_GetData(item);
        if (ATX_String_Equals(&header->name, name, ATX_TRUE)) {
            /* found a match */
            return &header->value;
        }
        item = ATX_ListItem_GetNext(item);
    }

    /* not found */
    return NULL;
}

/*----------------------------------------------------------------------
|    ATX_HttpMessage_SetProtocol
+---------------------------------------------------------------------*/
ATX_Result
ATX_HttpMessage_SetProtocol(ATX_HttpMessage* message,
                            ATX_CString      protocol)
{
    /* copy the new protocol */
    ATX_String_Assign(&message->protocol, protocol);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_HttpMessage_GetProtocol
+---------------------------------------------------------------------*/
const ATX_String*
ATX_HttpMessage_GetProtocol(const ATX_HttpMessage* message)
{
    return &message->protocol;
}

/*----------------------------------------------------------------------
|    ATX_HttpMessage_SetBody
+---------------------------------------------------------------------*/
ATX_Result
ATX_HttpMessage_SetBody(ATX_HttpMessage* self,
                        ATX_InputStream* stream,
                        ATX_Size         content_length)
{
    ATX_RELEASE_OBJECT(self->body);
    self->body = stream;
    ATX_REFERENCE_OBJECT(stream);

    /* recompute the content length header */
    if (stream) {
        char length_string[32];
        ATX_Http_NumToAscii(length_string, content_length);
        ATX_HttpMessage_SetHeader(self, ATX_HTTP_HEADER_CONTENT_LENGTH, 
                                  length_string);
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_HttpMessage_GetBody
+---------------------------------------------------------------------*/
ATX_Result
ATX_HttpMessage_GetBody(const ATX_HttpMessage* self,
                        ATX_InputStream*       stream,
                        ATX_Size*              content_length)
{
    /* return a reference to the stream */
    if (stream) {
        stream = self->body;
        ATX_REFERENCE_OBJECT(stream);
    }

    /* return the content length */
    if (content_length) {
        const ATX_String* length_string = 
            ATX_HttpMessage_GetHeader(self, ATX_HTTP_HEADER_CONTENT_LENGTH);
        if (length_string && self->body) {
            int length = 0;
            if (ATX_SUCCEEDED(ATX_String_ToInteger(length_string, &length, ATX_TRUE))) {
                *content_length = (ATX_Size)length;
            }
        } else {
            *content_length = 0;
        }
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_HttpMessage_Emit
+---------------------------------------------------------------------*/
static ATX_Result
ATX_HttpMessage_Emit(const ATX_HttpMessage* message, ATX_OutputStream* stream)
{
    ATX_ListItem* item = ATX_List_GetFirstItem(message->headers);

    /* output the headers */
    while (item) {
        ATX_HttpHeader* header = ATX_ListItem_GetData(item);
        if (header && 
            !ATX_String_IsEmpty(&header->name) && 
            !ATX_String_IsEmpty(&header->value)) {
            ATX_OutputStream_WriteString(stream, ATX_CSTR(header->name));
            ATX_OutputStream_Write(stream, ": ", 2, NULL);
            ATX_OutputStream_WriteLine(stream, ATX_CSTR(header->value));
            ATX_LOG_FINE_2("ATX_HttpMessage::Emit - %s: %s", ATX_CSTR(header->name), ATX_CSTR(header->value));
        }
        item = ATX_ListItem_GetNext(item);
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_HttpRequest_Create
+---------------------------------------------------------------------*/
ATX_Result 
ATX_HttpRequest_Create(ATX_CString       method, 
                       ATX_CString       url,
                       ATX_HttpRequest** request)
{
    ATX_Result result;

    /* allocate a new object */
    *request = (ATX_HttpRequest*)ATX_AllocateZeroMemory(sizeof(ATX_HttpRequest));
    if (*request == NULL) {
        return ATX_ERROR_OUT_OF_MEMORY;
    }

    /* construct the base object */
    result = ATX_HttpMessage_Construct(&(*request)->base);
    if (ATX_FAILED(result)) {
        ATX_FreeMemory((void*)request);
        return result;
    }

    /* construct the object */
    (*request)->method = ATX_String_Create(method);
    result = ATX_HttpUrl_Construct(&(*request)->url, url);
    if (ATX_FAILED(result)) {
        ATX_HttpMessage_Destruct(&(*request)->base);
        ATX_FreeMemory((void*)request);
        return result;
    }

    /* set the host header */
    if (!ATX_String_IsEmpty(&(*request)->url.host)) {
        ATX_HttpMessage_SetHeader((ATX_HttpMessage*)(*request), 
                                  ATX_HTTP_HEADER_HOST, 
                                  ATX_CSTR((*request)->url.host));
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_HttpRequest_Destroy
+---------------------------------------------------------------------*/
ATX_Result
ATX_HttpRequest_Destroy(ATX_HttpRequest* request)
{
    /* destruct the base object */
    ATX_HttpMessage_Destruct(&request->base);

    /* destruct the object */
    ATX_String_Destruct(&request->method);
    ATX_HttpUrl_Destruct(&request->url);

    /* free the object */
    ATX_FreeMemory(request);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_HttpRequest_Emit
+---------------------------------------------------------------------*/
ATX_Result
ATX_HttpRequest_Emit(const ATX_HttpRequest* request, ATX_OutputStream* stream)
{
    /* check that we have all we need */
    if (ATX_String_IsEmpty(&request->method) || 
        ATX_String_IsEmpty(&request->base.protocol)) {
        return ATX_ERROR_INVALID_PARAMETERS;
    }

    /* output the request line */
    ATX_OutputStream_WriteString(stream, ATX_CSTR(request->method));
    ATX_OutputStream_Write(stream, " ", 1, NULL);
    ATX_OutputStream_WriteString(stream, ATX_CSTR(request->url.path));
    ATX_OutputStream_Write(stream, " ", 1, NULL);
    ATX_OutputStream_WriteString(stream, ATX_CSTR(request->base.protocol));

    /* output the headers */
    ATX_HttpMessage_Emit(&request->base, stream);

    /* terminating line */
    ATX_OutputStream_Write(stream, "\r\n", 2, NULL);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_HttpResponse_Parse
+---------------------------------------------------------------------*/
static ATX_Result 
ATX_HttpResponse_Parse(ATX_HttpResponse* response, ATX_InputStream* stream)
{
    char        buffer[ATX_HTTP_MAX_LINE_SIZE+1];
    char*       line = buffer;
    char*       find;
    ATX_Boolean header_pending = ATX_FALSE;
    ATX_String  header_name = ATX_EMPTY_STRING;
    ATX_String  header_value = ATX_EMPTY_STRING;
    ATX_Result  result;

    /* get the first line from the stream */
    result = ATX_InputStream_ReadLine(stream, line, sizeof(buffer), NULL);
    if (ATX_FAILED(result)) return result;

    /* get the protocol */
    find = (char*)ATX_Http_FindChar(line, ' ');
    if (find == NULL) {
        return ATX_ERROR_INVALID_SYNTAX;
    }
    *find = '\0';
    ATX_String_Assign(&response->base.protocol, line);

    /* get the status code */
    line = (char*)ATX_Http_SkipWhitespace(find+1);
    find = (char*)ATX_Http_FindChar(line, ' ');
    if (find == NULL) {
        return ATX_ERROR_INVALID_SYNTAX;
    }
    *find = '\0';
    if (ATX_StringLength(line) != 3) {
        return ATX_ERROR_INVALID_SYNTAX;
    }
    {
        int i;
        response->status_code = 0;
        for (i=0; i<3; i++) {
            if (line[i] < '0' || line[i] > '9') {
                return ATX_ERROR_INVALID_SYNTAX;
            }
            response->status_code *= 10;
            response->status_code += line[i]-'0';
        }
    }

    /* the rest is the reason phrase */
    line = (char*)ATX_Http_SkipWhitespace(find+1);
    ATX_String_Assign(&response->reason_phrase, line);

    /* parse headers until an empty line or end of stream */
    do {
        /* read a line */
        result = ATX_InputStream_ReadLine(stream, line, sizeof(buffer), NULL);
        if (ATX_FAILED(result)) break;

        /* stop if line is empty */
        if (line[0] == '\0' || line[0] == '\r' || line[0] == '\n') {
            if (header_pending) {
                ATX_String_TrimWhitespace(&header_value);
                ATX_HttpMessage_SetHeader((ATX_HttpMessage*)response, 
                                          ATX_CSTR(header_name), 
                                          ATX_CSTR(header_value));
                ATX_LOG_FINE_2("ATX_HttpResponse::Parse - %s: %s",
                               ATX_CSTR(header_name),
                               ATX_CSTR(header_value));
            }
            break;
        }

        /* process the line */
        if ((line[0] == ' ' || line[0] == '\t') && header_pending) {
            /* this is a line continuation */
            ATX_String_Append(&header_value, line+1);
        } else {
            /* this is a new header */
            const char* name;
            const char* value;

            /* add the pending header to the list */
            if (header_pending) {
                ATX_String_TrimWhitespace(&header_value);
                ATX_HttpMessage_SetHeader((ATX_HttpMessage*)response, 
                                          ATX_CSTR(header_name), 
                                          ATX_CSTR(header_value));
                ATX_LOG_FINE_2("ATX_HttpResponse::Parse - %s: %s",
                               ATX_CSTR(header_name),
                               ATX_CSTR(header_value));
            }

            /* parse header name */
            name = ATX_Http_SkipWhitespace(line);
            value = ATX_Http_FindChar(name, ':');
            ATX_String_AssignN(&header_name, name, (ATX_Size)(value-name));
            value = ATX_Http_SkipWhitespace(value+1);
            ATX_String_Assign(&header_value, value);

            /* don't add the header now, it could be continued */
            header_pending = ATX_TRUE;
        }
    } while(ATX_SUCCEEDED(result));

    /* keep a reference to the stream */
    response->base.body = stream;
    ATX_REFERENCE_OBJECT(stream);

    /* cleanup */
    ATX_String_Destruct(&header_name);
    ATX_String_Destruct(&header_value);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_HttpResponse_CreateFromStream
+---------------------------------------------------------------------*/
ATX_Result 
ATX_HttpResponse_CreateFromStream(ATX_InputStream*   stream,
                                  ATX_HttpResponse** response)
{
    ATX_Result result;

    /* allocate a new object */
    *response = (ATX_HttpResponse*)ATX_AllocateZeroMemory(sizeof(ATX_HttpResponse));
    if (*response == NULL) {
        return ATX_ERROR_OUT_OF_MEMORY;
    }

    /* construct the base object */
    result = ATX_HttpMessage_Construct(&(*response)->base);
    if (ATX_FAILED(result)) {
        ATX_FreeMemory((void*)*response);
        *response = NULL;
        return result;
    }

    /* parse the response from the stream */
    result = ATX_HttpResponse_Parse(*response, stream);
    if (ATX_FAILED(result)) {
        ATX_HttpResponse_Destroy(*response);
        *response = NULL;
        return result;
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_HttpResponse_Destroy
+---------------------------------------------------------------------*/
ATX_Result
ATX_HttpResponse_Destroy(ATX_HttpResponse* response)
{
    /* destruct the base object */
    ATX_HttpMessage_Destruct(&response->base);

    /* destruct the object */
    ATX_String_Destruct(&response->reason_phrase);

    /* free the object */
    ATX_FreeMemory(response);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_HttpResponse_Emit
+---------------------------------------------------------------------*/
extern ATX_Result
ATX_HttpResponse_Emit(const ATX_HttpResponse* response,
                      ATX_OutputStream*       stream)
{
    /* check that we have what we need */
    if (ATX_String_IsEmpty(&response->base.protocol)) return ATX_ERROR_INVALID_PARAMETERS;
    if (response->status_code >= 1000) return ATX_ERROR_INVALID_PARAMETERS;

    /* output response line */
    ATX_OutputStream_WriteString(stream, ATX_CSTR(response->base.protocol));
    {
        char code_string[5];
        unsigned  code_int = response->status_code;
        code_string[0] = ' ';
        code_string[1] = '0' + code_int/100; 
        code_int -= 100*(code_int/100);
        code_string[2] = '0' + code_int/10;
        code_int -= 10*(code_int/10);
        code_string[3] = '0' + code_int;
        code_string[4] = ' ';
        ATX_OutputStream_Write(stream, code_string, 5, NULL);
    }
    ATX_OutputStream_WriteString(stream, ATX_CSTR(response->reason_phrase));
    ATX_OutputStream_Write(stream, "\r\n", 2, NULL);

    /* output the rest of the message */
    return ATX_HttpMessage_Emit((ATX_HttpMessage*)response, stream);
}

/*----------------------------------------------------------------------
|    ATX_HttpResponse_GetStatusCode
+---------------------------------------------------------------------*/
ATX_UInt32
ATX_HttpResponse_GetStatusCode(const ATX_HttpResponse* response)
{
    return response->status_code;
}

/*----------------------------------------------------------------------
|    ATX_HttpResponse_GetReasonPhrase
+---------------------------------------------------------------------*/
const ATX_String*
ATX_HttpResponse_GetReasonPhrase(const ATX_HttpResponse* response)
{
    return &response->reason_phrase;
}
