/*****************************************************************
|
|   BlueTune - TCP Network Stream
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltTypes.h"
#include "BltTcpNetworkStream.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.input.network.tcp")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_TCP_NETWORK_STREAM_DEFAULT_PORT    7875
#define BLT_TCP_NETWORK_STREAM_DEFAULT_TIMEOUT 15000 /* 15 secs */

/*----------------------------------------------------------------------
|   BLT_TcpNetworkStream_Create
+---------------------------------------------------------------------*/
BLT_Result 
BLT_TcpNetworkStream_Create(const char* name, ATX_InputStream** stream)
{
    ATX_Socket* sock;
    ATX_String  hostname = ATX_String_Create(name);
    ATX_UInt16  port = BLT_TCP_NETWORK_STREAM_DEFAULT_PORT;
    int         sep;
    ATX_Result  result = ATX_SUCCESS;

    /* default */
    *stream = NULL;

    /* parse the hostname/port */
    sep = ATX_String_FindCharFrom(&hostname, ':', 6);
    if (sep > 0) {
        /* we have a port number */
        int port_long = 0;
        result = ATX_ParseInteger(name+sep+1, &port_long, ATX_FALSE);
        if (ATX_FAILED(result)) {
            ATX_LOG_WARNING("BLT_TcpNetworkStream_Create - invalid port spec");
            goto end;
        }
        port = (ATX_UInt16)port_long;
        ATX_String_SetLength(&hostname, sep);
    }

    /* create a socket */
    result = ATX_TcpClientSocket_Create(&sock);
    if (ATX_FAILED(result)) goto end;
    
    /* connect */
    ATX_LOG_FINE_2("BLT_TcpNetworkStream_Create - connecting to %s:%d",
                   ATX_CSTR(hostname), port);
    result = ATX_Socket_ConnectToHost(sock, ATX_CSTR(hostname), port, BLT_TCP_NETWORK_STREAM_DEFAULT_TIMEOUT);
    if (ATX_FAILED(result)) {
        ATX_LOG_WARNING_1("BLT_TcpNetworkStream_Create - failed to connect (%d)", result);
        goto end;
    }
    ATX_LOG_FINE("BLT_TcpNetworkStream_Create - connected");

    /* return the input stream */
    result = ATX_Socket_GetInputStream(sock, stream);

    /* release the socket */
    ATX_DESTROY_OBJECT(sock);
    
end:
    ATX_String_Destruct(&hostname);
    return result;
}
