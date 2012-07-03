/*****************************************************************
|
|   Atomix - Sockets
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
|   includes
+---------------------------------------------------------------------*/
#include "AtxSockets.h"
#include "AtxUtils.h"
#include "AtxDefs.h"
#include "AtxResults.h"

/*----------------------------------------------------------------------
|   ATX_IpAddress_Reset
+---------------------------------------------------------------------*/
void
ATX_IpAddress_Reset(ATX_IpAddress* address) 
{
    (*address)[0] = (*address)[1] = (*address)[2] = (*address)[3] = 0;
}

/*----------------------------------------------------------------------
|   ATX_IpAddress_Copy
+---------------------------------------------------------------------*/
void
ATX_IpAddress_Copy(ATX_IpAddress* address, ATX_IpAddress* other) 
{
    (*address)[0] = (*other)[0];
    (*address)[1] = (*other)[1];
    (*address)[2] = (*other)[2];
    (*address)[3] = (*other)[3];
}

/*----------------------------------------------------------------------
|   ATX_IpAddress_SetFromLong
+---------------------------------------------------------------------*/
void
ATX_IpAddress_SetFromLong(ATX_IpAddress* address,
                          unsigned long  long_addr)
{
    (*address)[0] = (unsigned char)((long_addr >> 24) & 0xFF);
    (*address)[1] = (unsigned char)((long_addr >> 16) & 0xFF);
    (*address)[2] = (unsigned char)((long_addr >>  8) & 0xFF);
    (*address)[3] = (unsigned char)((long_addr      ) & 0xFF);
}

/*----------------------------------------------------------------------
|   ATX_IpAddress_AsLong
+---------------------------------------------------------------------*/
unsigned long
ATX_IpAddress_AsLong(const ATX_IpAddress* address)
{
    return 
        (((unsigned long)(*address)[0])<<24) |
        (((unsigned long)(*address)[1])<<16) |
        (((unsigned long)(*address)[2])<< 8) |
        (((unsigned long)(*address)[3]));
}

/*----------------------------------------------------------------------
|   ATX_IpAddress_Parse
+---------------------------------------------------------------------*/
ATX_Result
ATX_IpAddress_Parse(ATX_IpAddress* address, const char* name)
{
    unsigned int  fragment;
    ATX_Boolean   fragment_empty = ATX_FALSE;
    unsigned char parsed[4];
    unsigned int  accumulator;

    /* check the name */
    if (name == NULL) return ATX_ERROR_INVALID_PARAMETERS;

    /* clear the address */
    (*address)[0] = (*address)[1] = (*address)[2] = (*address)[3] = 0;

    /* parse */
    for (fragment = 0, accumulator = 0; fragment < 4; ++name) {
        if (*name == '\0' || *name == '.') {
            /* fragment terminator */
            if (fragment_empty) return ATX_ERROR_INVALID_SYNTAX;
            parsed[fragment++] = accumulator;
            if (*name == '\0') break;
            accumulator = 0;
            fragment_empty = ATX_TRUE;
        } else if (*name >= '0' && *name <= '9') {
            /* numerical character */
            accumulator = accumulator*10 + (*name - '0');
            if (accumulator > 255) return ATX_ERROR_INVALID_SYNTAX;
            fragment_empty = ATX_FALSE; 
        } else {
            /* invalid character */
            return ATX_ERROR_INVALID_SYNTAX;
        }
    }

    if (fragment == 4 && *name == '\0' && !fragment_empty) {
        (*address)[0] = parsed[0];
        (*address)[1] = parsed[1];
        (*address)[2] = parsed[2];
        (*address)[3] = parsed[3];
        return ATX_SUCCESS;
    } else {
        return ATX_ERROR_INVALID_SYNTAX;
    }
} 

/*----------------------------------------------------------------------
|   ATX_SocketAddress_Reset
+---------------------------------------------------------------------*/
void
ATX_SocketAddress_Reset(ATX_SocketAddress* address)
{
    ATX_IpAddress_Reset(&address->ip_address);
    address->port = 0;
}

/*----------------------------------------------------------------------
|   ATX_SocketAddress_Set
+---------------------------------------------------------------------*/
void
ATX_SocketAddress_Set(ATX_SocketAddress* address, 
                      ATX_IpAddress*     ip_address,
                      ATX_IpPort         ip_port)
{
    if (ip_address == NULL) {
        (address->ip_address)[0] = 0;
        (address->ip_address)[1] = 0;
        (address->ip_address)[2] = 0;
        (address->ip_address)[3] = 0;
    } else {
        (address->ip_address)[0] = (*ip_address)[0];
        (address->ip_address)[1] = (*ip_address)[1];
        (address->ip_address)[2] = (*ip_address)[2];
        (address->ip_address)[3] = (*ip_address)[3];
    }
    address->port = ip_port;
}

/*----------------------------------------------------------------------
|   ATX_Socket_ConnectToHost
+---------------------------------------------------------------------*/
ATX_Result 
ATX_Socket_ConnectToHost(ATX_Socket* socket,
                         ATX_CString host,
                         ATX_IpPort  port,
                         ATX_Timeout timeout)
{
    ATX_SocketAddress address;
    ATX_Result        result;

    /* resolve the name */
    result = ATX_IpAddress_ResolveName(&address.ip_address, host, timeout);
    if (ATX_FAILED(result)) return result;

    /* set the port */
    address.port = port;

    /* connect */
    return ATX_Socket_Connect(socket, &address, timeout);
}

