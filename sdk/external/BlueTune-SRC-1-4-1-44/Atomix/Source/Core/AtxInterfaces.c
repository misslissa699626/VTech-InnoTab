/*****************************************************************
|
|   Atomix - Interface Constants
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
#include "AtxInterfaces.h"

/*----------------------------------------------------------------------
|   interface constants
+---------------------------------------------------------------------*/
const ATX_InterfaceId ATX_INTERFACE_ID__ATX_Object           = {0x0001,0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID__ATX_Referenceable    = {0x0002,0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID__ATX_Destroyable      = {0x0003,0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID__ATX_Module           = {0x0004,0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID__ATX_InputStream      = {0x0005,0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID__ATX_OutputStream     = {0x0006,0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID__ATX_Socket           = {0x0007,0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID__ATX_DatagramSocket   = {0x0008,0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID__ATX_ServerSocket     = {0x0009,0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID__ATX_Properties       = {0x000A,0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID__ATX_PropertyListener = {0x000B,0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID__ATX_Iterator         = {0x000C,0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID__ATX_File             = {0x000D,0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID__ATX_StreamTransformer= {0x000E,0x0001};
