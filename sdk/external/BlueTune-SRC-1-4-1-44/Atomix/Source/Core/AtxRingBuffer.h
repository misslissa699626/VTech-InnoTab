/*****************************************************************
|
|   Atomix - Ring Buffers
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

#ifndef _ATX_RING_BUFFER_H_
#define _ATX_RING_BUFFER_H_

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "AtxTypes.h"
#include "AtxDefs.h"
#include "AtxResults.h"
#include "AtxUtils.h"
#include "AtxInterfaces.h"

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct ATX_RingBuffer ATX_RingBuffer;

/*----------------------------------------------------------------------
|    prototypes
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

ATX_Result     ATX_RingBuffer_Create(ATX_Size size, ATX_RingBuffer** buffer);
ATX_Result     ATX_RingBuffer_Destroy(ATX_RingBuffer* ring);
ATX_Size       ATX_RingBuffer_GetSpace(ATX_RingBuffer* ring);
ATX_Size       ATX_RingBuffer_GetContiguousSpace(ATX_RingBuffer* ring);
ATX_Size       ATX_RingBuffer_GetAvailable(ATX_RingBuffer* ring);
ATX_Size       ATX_RingBuffer_GetContiguousAvailable(ATX_RingBuffer* ring);
ATX_UInt8      ATX_RingBuffer_ReadByte(ATX_RingBuffer* ring);
ATX_UInt8      ATX_RingBuffer_PeekByte(ATX_RingBuffer* ring, 
                                       ATX_Size        offset);
ATX_ByteBuffer ATX_RingBuffer_GetIn(ATX_RingBuffer* ring);
ATX_ByteBuffer ATX_RingBuffer_GetOut(ATX_RingBuffer* ring);
ATX_Result     ATX_RingBuffer_Read(ATX_RingBuffer* ring, 
                                   ATX_ByteBuffer  buffer,
                                   ATX_Size        size);
ATX_Result     ATX_RingBuffer_Write(ATX_RingBuffer* ring, 
                                    ATX_ByteBuffer  buffer,
                                    ATX_Size        size);
ATX_Result     ATX_RingBuffer_MoveIn(ATX_RingBuffer* ring, ATX_Offset offset);
ATX_Result     ATX_RingBuffer_MoveOut(ATX_RingBuffer* ring, ATX_Offset offset);
ATX_Result     ATX_RingBuffer_Reset(ATX_RingBuffer* ring);

#ifdef __cplusplus
}
#endif

#endif /* _ATX_RING_BUFFER_H_ */
