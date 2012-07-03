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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "AtxDataBuffer.h"
#include "AtxResults.h"
#include "AtxUtils.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
struct ATX_DataBuffer {
    ATX_Boolean buffer_is_local;
    ATX_Byte*   buffer;
    ATX_Size    buffer_size;
    ATX_Size    data_size;
};

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define ATX_DATA_BUFFER_EXTRA_GROW_SPACE 256

/*----------------------------------------------------------------------
|   ATX_DataBuffer_Create
+---------------------------------------------------------------------*/
ATX_Result 
ATX_DataBuffer_Create(ATX_Size size, ATX_DataBuffer** buffer)
{
    /* allocate the object */
    *buffer = ATX_AllocateZeroMemory(sizeof(ATX_DataBuffer));
    if (*buffer == NULL) return ATX_ERROR_OUT_OF_MEMORY;

    /* construct the object */
    (*buffer)->buffer_is_local = ATX_TRUE;

    /* allocate the buffer */
    if (size) {
        (*buffer)->buffer_size = size;
        (*buffer)->buffer = ATX_AllocateMemory(size);
        if ((*buffer)->buffer == NULL) {
            ATX_FreeMemory((void*)(*buffer));
            return ATX_ERROR_OUT_OF_MEMORY;
        }
    }
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_DataBuffer_Clone
+---------------------------------------------------------------------*/
ATX_Result
ATX_DataBuffer_Clone(const ATX_DataBuffer* self, ATX_DataBuffer** clone)
{
    /* create a clone with a buffer of the same size */
    ATX_CHECK(ATX_DataBuffer_Create(self->data_size, clone));

    /* copy the data */
    ATX_CHECK(ATX_DataBuffer_SetData(*clone, 
                                     self->buffer, 
                                     self->data_size));
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_DataBuffer_Destroy
+---------------------------------------------------------------------*/
ATX_Result 
ATX_DataBuffer_Destroy(ATX_DataBuffer* self)
{
    /* free the buffer */
    if (self->buffer_is_local) ATX_FreeMemory((void*)self->buffer);

    /* free the object */
    ATX_FreeMemory((void*)self);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_DataBuffer_ReallocateBuffer
+---------------------------------------------------------------------*/
static ATX_Result
ATX_DataBuffer_ReallocateBuffer(ATX_DataBuffer* self, ATX_Size size)
{
    ATX_Byte* new_buffer;

    /* check that the existing data fits */
    if (self->data_size > size) return ATX_ERROR_INVALID_PARAMETERS;

    /* allocate a new buffer */
    new_buffer = (ATX_Byte*)ATX_AllocateMemory(size);
    if (new_buffer == NULL) return ATX_ERROR_OUT_OF_MEMORY;

    /* copy the contents of the previous buffer, if any */
    if (self->buffer && self->data_size) {
        ATX_CopyMemory(new_buffer, self->buffer, self->data_size);
    }

    /* destroy the previous buffer */
    ATX_FreeMemory((void*)self->buffer);

    /* use the new buffer */
    self->buffer = new_buffer;
    self->buffer_size = size;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_DataBuffer_SetBuffer
+---------------------------------------------------------------------*/
ATX_Result 
ATX_DataBuffer_SetBuffer(ATX_DataBuffer* self,
                         ATX_Byte*       buffer, 
                         ATX_Size        buffer_size)
{
    if (self->buffer_is_local) {
        /* destroy the local buffer */
        ATX_FreeMemory((void*)self->buffer);
    }

    /* we're now using an external buffer */
    self->buffer_is_local = ATX_FALSE;
    self->buffer = buffer;
    self->buffer_size = buffer_size;
    self->data_size = 0;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_DataBuffer_SetBufferSize
+---------------------------------------------------------------------*/
ATX_Result 
ATX_DataBuffer_SetBufferSize(ATX_DataBuffer* self,
                             ATX_Size        buffer_size)
{
    if (self->buffer_is_local) {
        return ATX_DataBuffer_ReallocateBuffer(self, buffer_size);
    } else {
        /* cannot change an external buffer */
        return ATX_ERROR_NOT_SUPPORTED; 
    }
}

/*----------------------------------------------------------------------
|   ATX_DataBuffer_Reserve
+---------------------------------------------------------------------*/
ATX_Result 
ATX_DataBuffer_Reserve(ATX_DataBuffer* self, ATX_Size size)
{
    if (self->buffer_size >= size) return ATX_SUCCESS;
    
    /* try doubling the size */
    {
        ATX_Size new_size = self->buffer_size*2;
        if (new_size < size) new_size = size + ATX_DATA_BUFFER_EXTRA_GROW_SPACE;
        return ATX_DataBuffer_SetBufferSize(self, new_size);
    }
}

/*----------------------------------------------------------------------
|   ATX_DataBuffer_GetBufferSize
+---------------------------------------------------------------------*/
ATX_Size   
ATX_DataBuffer_GetBufferSize(const ATX_DataBuffer* self)
{
    return self->buffer_size;
}

/*----------------------------------------------------------------------
|   ATX_DataBuffer_GetData
+---------------------------------------------------------------------*/
const ATX_Byte*  
ATX_DataBuffer_GetData(const ATX_DataBuffer* self)
{
    return self->buffer;
}

/*----------------------------------------------------------------------
|   ATX_DataBuffer_UseData
+---------------------------------------------------------------------*/
ATX_Byte* 
ATX_DataBuffer_UseData(ATX_DataBuffer* self)
{
    return self->buffer;
}

/*----------------------------------------------------------------------
|   ATX_DataBuffer_GetDataSize
+---------------------------------------------------------------------*/
ATX_Size   
ATX_DataBuffer_GetDataSize(const ATX_DataBuffer* self)
{
    return self->data_size;
}

/*----------------------------------------------------------------------
|   ATX_DataBuffer_SetDataSize
+---------------------------------------------------------------------*/
ATX_Result 
ATX_DataBuffer_SetDataSize(ATX_DataBuffer* self, ATX_Size size)
{
    if (size > self->buffer_size) {
        /* the buffer is too small, we need to reallocate it */
        if (self->buffer_is_local) {
            ATX_CHECK(ATX_DataBuffer_ReallocateBuffer(self, size));
        } else { 
            /* we cannot reallocate an external buffer */
            return ATX_ERROR_NOT_SUPPORTED;
        }
    }
    self->data_size = size;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_DataBuffer_SetData
+---------------------------------------------------------------------*/
ATX_Result 
ATX_DataBuffer_SetData(ATX_DataBuffer* self, 
                       const ATX_Byte* data,
                       ATX_Size        data_size)
{
    if (data_size > self->buffer_size) {
        if (self->buffer_is_local) {
            ATX_CHECK(ATX_DataBuffer_ReallocateBuffer(self, data_size));
        } else {
            return ATX_ERROR_OUT_OF_RESOURCES;
        }
    }
    ATX_CopyMemory(self->buffer, data, data_size);
    self->data_size = data_size;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_DataBuffer_Equals
+---------------------------------------------------------------------*/
ATX_Boolean
ATX_DataBuffer_Equals(const ATX_DataBuffer* self, 
                      const ATX_DataBuffer* other)
{
    /* true if both are NULL */
    if (self == NULL && other == NULL) return ATX_TRUE;

    /* not true if one of them is NULL */
    if (self == NULL || other == NULL) return ATX_FALSE;

    /* check that the sizes are the same */
    if (self->data_size != other->data_size) return ATX_FALSE;

    /* now compare the data */
    return ATX_MemoryEqual(self->buffer, 
                           other->buffer, 
                           self->data_size);
}

/*----------------------------------------------------------------------
|   ATX_DataBuffer_AppendData
+---------------------------------------------------------------------*/
ATX_Result
ATX_DataBuffer_AppendData(ATX_DataBuffer*   self,
                          const ATX_Byte*   data,
                          ATX_Size          data_size)
{
    ATX_Size   new_data_size = self->data_size + data_size;
    
    /* reserve the space and copy the appended data */
    ATX_CHECK(ATX_DataBuffer_Reserve(self, new_data_size));
    ATX_CopyMemory(self->buffer + self->data_size, data, data_size);
    self->data_size = new_data_size;
    
    return ATX_SUCCESS;
}
