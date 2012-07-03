/*****************************************************************
|
|      Atomix Tests - Http
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
|       includes
+---------------------------------------------------------------------*/
#include "Atomix.h"

/*----------------------------------------------------------------------
|       macros
+---------------------------------------------------------------------*/
#define CHECK_RESULT(r, m)                  \
    do {                                    \
        if (ATX_FAILED(r)) {                \
            ATX_Debug("%s (%d)\n", m, r);   \
            exit(1);                        \
        }                                   \
    } while(0)                              \

#if 0
/*----------------------------------------------------------------------
|       ConnectClient
+---------------------------------------------------------------------*/
static ATX_Result
ConnectClient(ATX_ClientSocket* client, 
              char*             hostname, 
              ATX_SocketPort    port,
              ATX_ByteStream*   stream)
{
    ATX_Socket socket;
    ATX_Result result;
    
    ATX_Debug("connecting to %s on port %d\n", hostname, port);

    /* connect client */
    result = ATX_ClientSocket_Connect(client, hostname, port, 10000);
    if (result != ATX_SUCCESS) {
        ATX_Debug("ERROR: connection failed (%d)\n", result);
        return result;
    }
    ATX_Debug("connected\n");

    /* cast to ATX_Socket interface */
    result = ATX_CAST_OBJECT(client, &socket, ATX_Socket);
    if (result != ATX_SUCCESS) {
        ATX_Debug("ERROR: client object does not implement the "
                  "ATX_Socket interface\n");
        return result;
    }

    /* get socket stream */
    result = ATX_Socket_GetStream(&socket, stream);
    if (result != ATX_SUCCESS) {
        ATX_Debug("ERROR: cannot get socket stream\n");
        return result;
    }

    /* check stream */
    if (ATX_OBJECT_IS_NULL(stream)) {
        ATX_Debug("ERROR: stream is NULL\n");
        return ATX_FAILURE;
    }

    return ATX_SUCCESS;
}

#endif

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int 
main(int argc, char** argv)
{
    ATX_HttpClient*   client;
    ATX_HttpRequest*  request;
    ATX_HttpResponse* response;
    ATX_InputStream*  response_body = NULL;
    ATX_Size          response_body_size = 0;
    const char*       url = "http://zebulon.bok.net/tmp/redirect";
    ATX_Result        result;


    /* command line args */
    if (argc == 2) url = argv[1];
    ATX_Debug("test url=%s\n", url);

    /* create a request */
    result = ATX_HttpRequest_Create(ATX_HTTP_METHOD_GET, url, &request);
    CHECK_RESULT(result, "ATX_HttpRequest_Create failed");
    
    /* create a client */
    result = ATX_HttpClient_Create(&client);
    CHECK_RESULT(result, "ATX_HttpClient_Create failed");

    /* send the request and get a response */
    result = ATX_HttpClient_SendRequest(client, request, &response);
    CHECK_RESULT(result, "ATX_HttpClient_SendRequest failed");

    /* print the response */
    ATX_Debug("StatusCode = %d\n", ATX_HttpResponse_GetStatusCode(response));
    ATX_Debug("ReasonPhrase = %s\n", 
              ATX_String_GetChars(ATX_HttpResponse_GetReasonPhrase(response)));
    ATX_HttpMessage_GetBody((const ATX_HttpMessage*)response, NULL, &response_body_size);
    ATX_Debug("BodySize = %d\n", response_body_size);


    /* cleanup */
    ATX_RELEASE_OBJECT(response_body);
    ATX_HttpResponse_Destroy(response);
    ATX_HttpRequest_Destroy(request);
    ATX_HttpClient_Destroy(client);

    return 0;
}

