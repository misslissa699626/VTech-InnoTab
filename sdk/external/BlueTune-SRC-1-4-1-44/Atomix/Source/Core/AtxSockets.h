/*****************************************************************
|
|   Atomix - Network Sockets 
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

#ifndef _ATX_SOCKETS_H_
#define _ATX_SOCKETS_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "AtxTypes.h"
#include "AtxStreams.h"
#include "AtxDataBuffer.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef unsigned int ATX_IpPort;
typedef unsigned char ATX_IpAddress[4];

typedef struct {
    ATX_IpAddress ip_address;
    ATX_IpPort    port;
} ATX_SocketAddress;

typedef struct {
    ATX_SocketAddress local_address;
    ATX_SocketAddress remote_address;
} ATX_SocketInfo;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define ATX_SOCKET_TIMEOUT_INFINITE         -1

#define ATX_ERROR_DISCONNECTED        (ATX_ERROR_BASE_SOCKETS - 0)
#define ATX_ERROR_HOST_UNKNOWN        (ATX_ERROR_BASE_SOCKETS - 1)
#define ATX_ERROR_SOCKET_FAILED       (ATX_ERROR_BASE_SOCKETS - 2)
#define ATX_ERROR_CONNECTION_REFUSED  (ATX_ERROR_BASE_SOCKETS - 3)
#define ATX_ERROR_CONNECTION_FAILED   (ATX_ERROR_BASE_SOCKETS - 4)
#define ATX_ERROR_CONNECTION_RESET    (ATX_ERROR_BASE_SOCKETS - 5)
#define ATX_ERROR_CONNECTION_ABORTED  (ATX_ERROR_BASE_SOCKETS - 6)
#define ATX_ERROR_TIMEOUT             (ATX_ERROR_BASE_SOCKETS - 7)
#define ATX_ERROR_BIND_FAILED         (ATX_ERROR_BASE_SOCKETS - 8)
#define ATX_ERROR_LISTEN_FAILED       (ATX_ERROR_BASE_SOCKETS - 9)
#define ATX_ERROR_ACCEPT_FAILED       (ATX_ERROR_BASE_SOCKETS - 10)
#define ATX_ERROR_SELECT_FAILED       (ATX_ERROR_BASE_SOCKETS - 11)
#define ATX_ERROR_ADDRESS_IN_USE      (ATX_ERROR_BASE_SOCKETS - 12)
#define ATX_ERROR_NETWORK_DOWN        (ATX_ERROR_BASE_SOCKETS - 13)
#define ATX_ERROR_NETWORK_UNREACHABLE (ATX_ERROR_BASE_SOCKETS - 14)
#define ATX_ERROR_WOULD_BLOCK         (ATX_ERROR_BASE_SOCKETS - 15)

/*----------------------------------------------------------------------
|   functions
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void ATX_IpAddress_Reset(ATX_IpAddress* address);
void ATX_IpAddress_SetFromLong(ATX_IpAddress* address,
                               unsigned long  long_addr);
unsigned long ATX_IpAddress_AsLong(const ATX_IpAddress* address);
ATX_Result ATX_IpAddress_Parse(ATX_IpAddress* address, ATX_CString name);
ATX_Result ATX_IpAddress_ResolveName(ATX_IpAddress* address,
                                     ATX_CString    name, 
                                     ATX_Timeout    timeout);
void ATX_IpAddress_Copy(ATX_IpAddress* address,
                        ATX_IpAddress* other);
void ATX_SocketAddress_Reset(ATX_SocketAddress* address);
void ATX_SocketAddress_Set(ATX_SocketAddress* address,
                           ATX_IpAddress*     ip_address,
                           ATX_IpPort         port);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*----------------------------------------------------------------------
|   ATX_Socket
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(ATX_Socket)
ATX_BEGIN_INTERFACE_DEFINITION(ATX_Socket)
    ATX_Result (*Bind)(ATX_Socket*              self, 
                       const ATX_SocketAddress* address);
    ATX_Result (*Connect)(ATX_Socket*              self, 
                          const ATX_SocketAddress* address,
                          ATX_Timeout              timeout);
    ATX_Result (*GetInputStream)(ATX_Socket*         self, 
                                 ATX_InputStream**   stream);
    ATX_Result (*GetOutputStream)(ATX_Socket*         self, 
                                  ATX_OutputStream**  stream);
    ATX_Result (*GetInfo)(ATX_Socket*         self, 
                          ATX_SocketInfo*     info);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define ATX_Socket_Bind(object, address) \
ATX_INTERFACE(object)->Bind(object, address)

#define ATX_Socket_Connect(object, address, timeout) \
ATX_INTERFACE(object)->Connect(object, \
                               address,              \
                               timeout)

#define ATX_Socket_GetInputStream(object, stream)\
ATX_INTERFACE(object)->GetInputStream(object, stream)

#define ATX_Socket_GetOutputStream(object, stream)\
ATX_INTERFACE(object)->GetOutputStream(object, stream)

#define ATX_Socket_GetInfo(object, info)\
ATX_INTERFACE(object)->GetInfo(object, info)

/*----------------------------------------------------------------------
|   functions
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

ATX_Result ATX_Socket_ConnectToHost(ATX_Socket* self,
                                    ATX_CString host,
                                    ATX_IpPort  port,
                                    ATX_Timeout timeout);
#ifdef __cplusplus
}
#endif /* __cplusplus */


/*----------------------------------------------------------------------
|   ATX_DatagramSocket
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(ATX_DatagramSocket)
ATX_BEGIN_INTERFACE_DEFINITION(ATX_DatagramSocket)
    ATX_Result (*Send)(ATX_DatagramSocket*         self,
                       const ATX_DataBuffer*       packet, 
                       const ATX_SocketAddress*    address);
    ATX_Result (*Receive)(ATX_DatagramSocket*         self,
                          ATX_DataBuffer*             packet, 
                          ATX_SocketAddress*          address);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define ATX_DatagramSocket_Send(object, packet, address) \
ATX_INTERFACE(object)->Send(object, packet, address)

#define ATX_DatagramSocket_Receive(object, packet, address) \
ATX_INTERFACE(object)->Receive(object, packet, address)

/*----------------------------------------------------------------------
|   ATX_ServerSocket
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(ATX_ServerSocket)
ATX_BEGIN_INTERFACE_DEFINITION(ATX_ServerSocket)
    ATX_Result (*Listen)(ATX_ServerSocket* self,
                         unsigned int      max_clients);
    ATX_Result (*WaitForNewClient)(ATX_ServerSocket* self,
                                   ATX_Socket**      client);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define ATX_ServerSocket_Listen(object, max_clients) \
ATX_INTERFACE(object)->Listen(object, max_clients)

#define ATX_ServerSocket_WaitForNewClient(object, client) \
ATX_INTERFACE(object)->WaitForNewClient(object, client)

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern ATX_Result ATX_TcpClientSocket_Create(ATX_Socket** bsd_socket);
extern ATX_Result ATX_TcpServerSocket_Create(ATX_ServerSocket** bsd_socket);
extern ATX_Result ATX_UdpSocket_Create(ATX_DatagramSocket** bsd_socket);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _ATX_SOCKETS_H_ */









