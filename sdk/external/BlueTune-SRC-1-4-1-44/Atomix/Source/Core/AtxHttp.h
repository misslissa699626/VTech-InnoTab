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

#ifndef _ATX_HTTP_H_
#define _ATX_HTTP_H_

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "AtxTypes.h"
#include "AtxDefs.h"
#include "AtxResults.h"
#include "AtxUtils.h"
#include "AtxInterfaces.h"
#include "AtxProperties.h"
#include "AtxStreams.h"

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct ATX_HttpClient   ATX_HttpClient;
typedef struct ATX_HttpMessage  ATX_HttpMessage;
typedef struct ATX_HttpRequest  ATX_HttpRequest;
typedef struct ATX_HttpResponse ATX_HttpResponse;

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define ATX_HTTP_METHOD_GET     "GET"
#define ATX_HTTP_METHOD_HEAD    "HEAD"
#define ATX_HTTP_METHOD_POST    "POST"

#define ATX_HTTP_CLIENT_OPTION_FOLLOW_REDIRECT "FollowRedirect"

/*----------------------------------------------------------------------
|    prototypes
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern ATX_Result 
ATX_HttpMessage_Create(ATX_HttpMessage** message);

extern ATX_Result
ATX_HttpMessage_Destroy(ATX_HttpMessage* message);

extern ATX_Result
ATX_HttpMessage_SetHeader(ATX_HttpMessage* message,
                          ATX_CString      name, 
                          ATX_CString      value);

extern const ATX_String*
ATX_HttpMessage_GetHeader(const ATX_HttpMessage* message,
                          ATX_CString            name);

extern ATX_Result
ATX_HttpMessage_SetProtocol(ATX_HttpMessage* message,
                            ATX_CString      protocol);

extern const ATX_String*
ATX_HttpMessage_GetProtocol(const ATX_HttpMessage* message);

extern ATX_Result
ATX_HttpMessage_SetBody(ATX_HttpMessage* message,
                        ATX_InputStream* stream,
                        ATX_Size         content_length);

extern ATX_Result
ATX_HttpMessage_GetBody(const ATX_HttpMessage* message,
                        ATX_InputStream*       stream,
                        ATX_Size*              content_length);

extern ATX_Result
ATX_HttpRequest_Create(ATX_CString       method, 
                       ATX_CString       uri,
                       ATX_HttpRequest** request);

extern ATX_Result
ATX_HttpRequest_Destroy(ATX_HttpRequest* request);

extern const ATX_String*
ATX_HttpRequest_GetMethod(const ATX_HttpRequest* request);

extern ATX_Result
ATX_HttpRequest_SetMethod(ATX_HttpRequest* request,
                          ATX_CString      method);

extern ATX_Result
ATX_HttpRequest_Emit(const ATX_HttpRequest* request,
                     ATX_OutputStream*      stream);

extern ATX_Result
ATX_HttpResponse_CreateFromStream(ATX_InputStream*   stream,
                                  ATX_HttpResponse** request);

extern ATX_Result
ATX_HttpResponse_Destroy(ATX_HttpResponse* response);

extern ATX_Result
ATX_HttpResponse_Emit(const ATX_HttpResponse* response, 
                      ATX_OutputStream*       stream);

extern ATX_UInt32
ATX_HttpResponse_GetStatusCode(const ATX_HttpResponse* response);

extern const ATX_String*
ATX_HttpResponse_GetReasonPhrase(const ATX_HttpResponse* response);

extern ATX_Result
ATX_HttpClient_Create(ATX_HttpClient** client);

extern ATX_Result
ATX_HttpClient_Destroy(ATX_HttpClient* client);

extern ATX_Result
ATX_HttpClient_SetOptionBool(ATX_HttpClient* client, 
                             ATX_CString     option,
                             ATX_Boolean     value);

extern ATX_Result
ATX_HttpClient_GetOptionBool(const ATX_HttpClient* client, 
                             ATX_CString           option,
                             ATX_Boolean*          value);

extern ATX_Result 
ATX_HttpClient_SendRequest(ATX_HttpClient*    client,
                           ATX_HttpRequest*   request,
                           ATX_HttpResponse** response);

#ifdef __cplusplus
}
#endif

#endif /* _ATX_HTTP_H_ */
