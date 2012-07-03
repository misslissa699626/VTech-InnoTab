/*****************************************************************
|
|   Atomix - Data Buffers
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

#ifndef _ATX_DATA_BUFFER_H_
#define _ATX_DATA_BUFFER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "AtxTypes.h"
#include "AtxDefs.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct ATX_DataBuffer ATX_DataBuffer;

/*----------------------------------------------------------------------
|   functions
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

ATX_Result ATX_DataBuffer_Create(ATX_Size size, ATX_DataBuffer** buffer);
ATX_Result ATX_DataBuffer_Clone(const ATX_DataBuffer* self,
                                ATX_DataBuffer**      clone);
ATX_Result ATX_DataBuffer_Destroy(ATX_DataBuffer* self);
ATX_Result ATX_DataBuffer_SetBuffer(ATX_DataBuffer* self,
                                    ATX_Byte*       buffer_memory, 
                                    ATX_Size        buffer_size);
ATX_Result ATX_DataBuffer_SetBufferSize(ATX_DataBuffer* self,
                                        ATX_Size        buffer_size);
ATX_Size   ATX_DataBuffer_GetBufferSize(const ATX_DataBuffer* self);
ATX_Result ATX_DataBuffer_Reserve(ATX_DataBuffer* self,
                                  ATX_Size        buffer_size);
const ATX_Byte*  ATX_DataBuffer_GetData(const ATX_DataBuffer* self);
ATX_Byte*  ATX_DataBuffer_UseData(ATX_DataBuffer* self);
ATX_Size   ATX_DataBuffer_GetDataSize(const ATX_DataBuffer* self);
ATX_Result ATX_DataBuffer_SetDataSize(ATX_DataBuffer* self, ATX_Size size);
ATX_Result ATX_DataBuffer_SetData(ATX_DataBuffer* self, 
                                  const ATX_Byte* data,
                                  ATX_Size        data_size);
ATX_Boolean ATX_DataBuffer_Equals(const ATX_DataBuffer* self,
                                  const ATX_DataBuffer* other);
ATX_Result  ATX_DataBuffer_AppendData(ATX_DataBuffer* self,
                                      const ATX_Byte* data,
                                      ATX_Size        data_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ATX_DATA_BUFFER_H_ */
