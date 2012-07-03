/*****************************************************************
|
|   Atomix - Streams Runtime Support
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
#include "AtxConfig.h"
#include "AtxTypes.h"
#include "AtxResults.h"
#include "AtxUtils.h"
#include "AtxStreams.h"
#include "AtxReferenceable.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(ATX_InputStream);
    ATX_IMPLEMENTS(ATX_Referenceable);

    /* members */
    ATX_Cardinal           reference_count;
    ATX_InputStream*       parent;
    ATX_StreamTransformer* transformer;
    ATX_LargeSize          size;
    ATX_Position           offset;
    ATX_Position           position;
} ATX_SubInputStream;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define ATX_INPUT_STREAM_LOAD_DEFAULT_READ_CHUNK 4096
#define ATX_INPUT_STREAM_LOAD_MAX_SIZE           0x100000

/*----------------------------------------------------------------------
|   ATX_InputStream_ReadLine
+---------------------------------------------------------------------*/
ATX_Result
ATX_InputStream_ReadLine(ATX_InputStream* self, 
                         char*            buffer, 
                         ATX_Size         size, 
                         ATX_Size*        chars_read)
{
    ATX_Result result;
    ATX_Size   total = 0;

    /* check parameters */
    if (buffer == NULL || size < 1) {
        return ATX_ERROR_INVALID_PARAMETERS;
    }

    /* read until EOF or newline */
    do {
        ATX_Size bytes_read;
        result = ATX_InputStream_Read(self, buffer, 1, &bytes_read);
        if (ATX_SUCCEEDED(result) && bytes_read == 1) {
            if (*buffer == '\n') {
                *buffer = '\0';
                if (chars_read) *chars_read = total;
                return ATX_SUCCESS;
            }  else if (*buffer == '\r') {
                continue;
            }
        } else {
            if (chars_read) *chars_read = total;
            if (result == ATX_ERROR_EOS) {
                *buffer = '\0';
                if (total != 0) {
                    return ATX_SUCCESS;
                } else {
                    return ATX_ERROR_EOS;
                }
            } else {
                return result;
            }
        }
        total++;
        buffer++;
    } while (total < size-1);
    
    /* terminate the line */
    *buffer = '\0';

    /* return what we have */
    if (chars_read) *chars_read = total;
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_InputStream_ReadLineString
+---------------------------------------------------------------------*/
ATX_Result
ATX_InputStream_ReadLineString(ATX_InputStream* self, 
                               ATX_String*      string,
                               ATX_Size         max_length)
{
    ATX_Result result;

    /* reset the string */
    ATX_String_SetLength(string, 0);

    /* read until EOF or newline */
    do {
        ATX_Size bytes_read;
        char     c;
        result = ATX_InputStream_Read(self, &c, 1, &bytes_read);
        if (ATX_SUCCEEDED(result) && bytes_read == 1) {
            if (c == '\n') {
                return ATX_SUCCESS;
            }  else if (c == '\r') {
                continue;
            }
        } else {
            if (result == ATX_ERROR_EOS) {
                return ATX_String_IsEmpty(string)?ATX_ERROR_EOS:ATX_SUCCESS;
            } else {
                return result;
            }
        }
        ATX_String_AppendChar(string, c);
    } while (ATX_String_GetLength(string)<max_length);

    return ATX_ERROR_NOT_ENOUGH_SPACE;
}

/*----------------------------------------------------------------------
|   ATX_InputStream_ReadUI16
+---------------------------------------------------------------------*/
ATX_Result
ATX_InputStream_ReadUI16(ATX_InputStream* self, ATX_UInt16* value)
{
    unsigned char buffer[2];
    
    /* read bytes from the stream */
    ATX_Result result;
    result = ATX_InputStream_ReadFully(self, (void*)buffer, 2);
    if (ATX_FAILED(result)) {
        *value = 0;
        return result;
    }
    
    /* convert bytes to value */
    *value = ATX_BytesToInt16Be(buffer);
    
    return ATX_SUCCESS;   
}

/*----------------------------------------------------------------------
|   ATX_InputStream_ReadUI32
+---------------------------------------------------------------------*/
ATX_Result
ATX_InputStream_ReadUI32(ATX_InputStream* self, ATX_UInt32* value)
{
    unsigned char buffer[4];

    /* read bytes from the stream */
    ATX_Result result;
    result = ATX_InputStream_ReadFully(self, (void*)buffer, 4);
    if (ATX_FAILED(result)) {
        *value = 0;
        return result;
    }

    /* convert bytes to value */
    *value = ATX_BytesToInt32Be(buffer);
    
    return ATX_SUCCESS;   
}

/*----------------------------------------------------------------------
|   ATX_InputStream_ReadUI64
+---------------------------------------------------------------------*/
ATX_Result
ATX_InputStream_ReadUI64(ATX_InputStream* self, ATX_UInt64* value)
{
    unsigned char buffer[8];

    /* read bytes from the stream */
    ATX_Result result;
    result = ATX_InputStream_ReadFully(self, (void*)buffer, 8);
    if (ATX_FAILED(result)) {
        *value = 0;
        return result;
    }

    /* convert bytes to value */
    *value = ATX_BytesToInt64Be(buffer);
    
    return ATX_SUCCESS;   
}

/*----------------------------------------------------------------------
|   ATX_InputStream_ReadFully
+---------------------------------------------------------------------*/
ATX_Result
ATX_InputStream_ReadFully(ATX_InputStream* self, 
                          ATX_Any          buffer, 
                          ATX_Size         bytes_to_read)
{
    ATX_Size   bytes_read;
    ATX_Result result;

    while (bytes_to_read) {
        result = ATX_InputStream_Read(self, buffer, bytes_to_read, &bytes_read);
        if (ATX_FAILED(result)) return result;
        if (bytes_read == 0) return ATX_ERROR_INTERNAL;
        bytes_to_read -= bytes_read;
        buffer = (ATX_Any)(((char*)buffer)+bytes_read);
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_InputStream_Skip
+---------------------------------------------------------------------*/
ATX_Result
ATX_InputStream_Skip(ATX_InputStream* self, ATX_Size count)
{
    ATX_Position position;
    ATX_Result   result;

    /* get the current location */
    result = ATX_InputStream_Tell(self, &position);
    if (ATX_FAILED(result)) return result;
    
    /* seek ahead */
    return ATX_InputStream_Seek(self, position+count);
}

/*----------------------------------------------------------------------
|   ATX_InputStream_Load
+---------------------------------------------------------------------*/
ATX_Result
ATX_InputStream_Load(ATX_InputStream* self, ATX_Size max_read, ATX_DataBuffer** buffer)
{
    ATX_Result    result;
    ATX_LargeSize total_bytes_read;
    ATX_LargeSize size = 0;

    /* create a buffer if none was given */
    if (*buffer == NULL) {
        ATX_CHECK(ATX_DataBuffer_Create(0, buffer));
    }

    /* reset the buffer */
    ATX_DataBuffer_SetDataSize(*buffer, 0);

    /* check args */
    if (max_read > ATX_INPUT_STREAM_LOAD_MAX_SIZE) {
        return ATX_ERROR_OUT_OF_RANGE;
    }

    /* try to get the stream size */
    if (ATX_FAILED(ATX_InputStream_GetSize(self, &size))) {
        size = max_read;
    }  else {
        if (size > ATX_INPUT_STREAM_LOAD_MAX_SIZE) {
            return ATX_ERROR_OUT_OF_RANGE;
        }
        if (max_read && max_read < size) size = max_read;
    }

    /* pre-allocate the buffer */
    if (size) ATX_CHECK(ATX_DataBuffer_Reserve(*buffer, (ATX_Size)size));

    /* read the data from the file */
    total_bytes_read = 0;
    do {
        ATX_LargeSize available = 0;
        ATX_LargeSize bytes_to_read;
        ATX_Size      bytes_read;
        ATX_Byte*     data;

        /* check if we know how much data is available */
        result = ATX_InputStream_GetAvailable(self, &available);
        if (ATX_SUCCEEDED(result) && available) {
            /* we know how much is available */
            bytes_to_read = available;
        } else {
            bytes_to_read = ATX_INPUT_STREAM_LOAD_DEFAULT_READ_CHUNK;
        }

        /* make sure we don't read more than what was asked */
        if (size != 0 && total_bytes_read+bytes_to_read>size) {
            bytes_to_read = size-total_bytes_read;
        }

        /* stop if we've read everything */
        if (bytes_to_read == 0) break;

        /* check that we don't read too much */
        if (total_bytes_read+bytes_to_read > ATX_INPUT_STREAM_LOAD_MAX_SIZE) {
            ATX_DataBuffer_SetBufferSize(*buffer, 0);
            return ATX_ERROR_OUT_OF_RANGE;
        }

        /* allocate space in the buffer */
        ATX_CHECK(ATX_DataBuffer_Reserve(*buffer, (ATX_Size)(total_bytes_read+bytes_to_read)));

        /* read the data */
        data = ATX_DataBuffer_UseData(*buffer)+total_bytes_read;
        result = ATX_InputStream_Read(self, (void*)data, (ATX_Size)bytes_to_read, &bytes_read);
        if (ATX_SUCCEEDED(result) && bytes_read != 0) {
            total_bytes_read += bytes_read;
            ATX_DataBuffer_SetDataSize(*buffer, (ATX_Size)total_bytes_read);
        }
    } while(ATX_SUCCEEDED(result) && (size==0 || total_bytes_read < size));

    if (result == ATX_ERROR_EOS) {
        return ATX_SUCCESS;
    } else {
        return result;
    }
}

/*----------------------------------------------------------------------
|   ATX_OutputStream_WriteFully
+---------------------------------------------------------------------*/
ATX_Result
ATX_OutputStream_WriteFully(ATX_OutputStream* self, 
                            const void*       buffer, 
                            ATX_Size          bytes_to_write)
{
    /* shortcut */
    if (bytes_to_write == 0) return ATX_SUCCESS;

    /* write until failure */
    while (bytes_to_write) {
        ATX_Size bytes_written;
        ATX_Result result = ATX_OutputStream_Write(self, buffer, bytes_to_write, &bytes_written);
        if (ATX_FAILED(result)) return result;
        if (bytes_written == 0) return ATX_ERROR_INTERNAL;
        ATX_ASSERT(bytes_written <= bytes_to_write);
        bytes_to_write -= bytes_written;
        buffer = (const void*)(((const ATX_Byte*)buffer)+bytes_written);
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_OutputStream_WriteString
+---------------------------------------------------------------------*/
ATX_Result
ATX_OutputStream_WriteString(ATX_OutputStream* self, ATX_CString string)
{
    /* shortcut */
    ATX_Size string_length;
    if (string == NULL || (string_length = ATX_StringLength(string)) == 0) {
        return ATX_SUCCESS;
    }

    /* write the string */
    return ATX_OutputStream_WriteFully(self, 
                                       (const void*)string, 
                                       string_length);
}

/*----------------------------------------------------------------------
|   ATX_OutputStream_WriteLine
+---------------------------------------------------------------------*/
ATX_Result
ATX_OutputStream_WriteLine(ATX_OutputStream* self, ATX_CString string)
{
    ATX_Result result;
    result = ATX_OutputStream_WriteString(self, string);
    if (ATX_FAILED(result)) return result;
    result = ATX_OutputStream_WriteFully(self, (const void*)"\r\n", 2);
    if (ATX_FAILED(result)) return result;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(ATX_SubInputStream, ATX_InputStream)
ATX_DECLARE_INTERFACE_MAP(ATX_SubInputStream, ATX_Referenceable)

/*----------------------------------------------------------------------
|   ATX_SubInputStream_Create
+---------------------------------------------------------------------*/
ATX_Result
ATX_SubInputStream_Create(ATX_InputStream*       parent, 
                          ATX_Position           offset,
                          ATX_LargeSize          size,
                          ATX_StreamTransformer* transformer,
                          ATX_InputStream**      object)
{ 
    ATX_SubInputStream* stream;
    ATX_Result          result;

    /* allocate new object */
    stream = (ATX_SubInputStream*)ATX_AllocateZeroMemory(sizeof(ATX_SubInputStream));
    if (stream == NULL) {
        *object = NULL;
        return ATX_ERROR_OUT_OF_MEMORY;
    }

    /* construct object */
    stream->reference_count = 1;
    stream->parent          = parent;
    if (transformer) {
        stream->transformer = transformer;
    }

    /* check the size and offset */
    {
        ATX_LargeSize parent_size;

        stream->offset   = offset;
        stream->size     = size;
        stream->position = 0;

        result = ATX_InputStream_GetSize(parent, &parent_size);
        if (ATX_SUCCEEDED(result)) {
            if (parent_size > 0) {
                /* parent size known */
                if ((ATX_LargeSize)offset >= parent_size) {
                    *object = NULL;
                    ATX_FreeMemory((void*)stream);
                    return ATX_ERROR_INVALID_PARAMETERS;
                }
                if (size == 0) {
                    /* unspecified size for the substream */
                    stream->size = parent_size - offset;
                } else {
                    if (size > parent_size - offset) {
                        stream->size = parent_size - offset;
                    }
                }
            }
        }
    }
    
    /* seek to the start point */
    result = ATX_InputStream_Seek(parent, offset);
    if (ATX_FAILED(result)) stream->offset = 0;

    /* keep a reference to the parent stream */
    ATX_REFERENCE_OBJECT(parent);

    /* setup the interfaces */
    ATX_SET_INTERFACE(stream, ATX_SubInputStream, ATX_InputStream);
    ATX_SET_INTERFACE(stream, ATX_SubInputStream, ATX_Referenceable);
    *object = &ATX_BASE(stream, ATX_InputStream);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_SubInputStream_Destroy
+---------------------------------------------------------------------*/
static ATX_Result
ATX_SubInputStream_Destroy(ATX_SubInputStream* self)
{
    /* release the reference to the parent */
    ATX_RELEASE_OBJECT(self->parent);
    ATX_FreeMemory((void*)self);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_SubInputStream_Read
+---------------------------------------------------------------------*/
ATX_METHOD
ATX_SubInputStream_Read(ATX_InputStream* _self,
                        ATX_Any          buffer, 
                        ATX_Size         bytes_to_read, 
                        ATX_Size*        bytes_read)
{
    ATX_SubInputStream* self = ATX_SELF(ATX_SubInputStream, ATX_InputStream);
    ATX_Size            local_read;
    ATX_Result          result;

    /* clip the request */
    if (self->size > 0) {
        if (bytes_to_read > self->size - self->position) {
            bytes_to_read = (ATX_Size)(self->size - self->position);
            if (bytes_to_read == 0) {
                if (bytes_read != NULL) *bytes_read = 0;
                return ATX_ERROR_EOS;
            }
        }
    }

    /* read from the parent */
    result = ATX_InputStream_Read(self->parent, 
                                  buffer, 
                                  bytes_to_read, 
                                  &local_read);
    if (ATX_SUCCEEDED(result)) {
        /* transform if there is a transformer */
        if (self->transformer) {
            ATX_StreamTransformer_Transform(self->transformer, buffer, local_read);
        }

        /* update the position */
        self->position += local_read;

        /* return the number of bytes read */
        if (bytes_read) *bytes_read = local_read;
        return ATX_SUCCESS;
    } else {
        if (bytes_read) *bytes_read = 0;
        return result;
    }
}

/*----------------------------------------------------------------------
|   ATX_SubInputStream_Seek
+---------------------------------------------------------------------*/
ATX_METHOD
ATX_SubInputStream_Seek(ATX_InputStream* _self, 
                        ATX_Position     where)
{
    ATX_SubInputStream* self = ATX_SELF(ATX_SubInputStream, ATX_InputStream);
    ATX_Position        parent_offset;
    ATX_Result          result;

    /* shortcut */
    if (where == self->position) return ATX_SUCCESS;

    /* compute the parent offset */
    parent_offset = where + self->offset;
    
    /* seek */
    result = ATX_InputStream_Seek(self->parent, parent_offset);
    if (ATX_SUCCEEDED(result)) {
        self->position = where;
    }
    return result;
}

/*----------------------------------------------------------------------
|   ATX_SubInputStream_Tell
+---------------------------------------------------------------------*/
ATX_METHOD
ATX_SubInputStream_Tell(ATX_InputStream* _self, 
                        ATX_Position*    where)
{
    ATX_SubInputStream* self = ATX_SELF(ATX_SubInputStream, ATX_InputStream);
    if (where) *where = self->position;
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_SubInputStream_GetSize
+---------------------------------------------------------------------*/
ATX_METHOD
ATX_SubInputStream_GetSize(ATX_InputStream* _self,
                           ATX_LargeSize*   size)
{
    ATX_SubInputStream* self = ATX_SELF(ATX_SubInputStream, ATX_InputStream);
    if (size) *size = self->size;
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_SubInputStream_GetAvailable
+---------------------------------------------------------------------*/
ATX_METHOD
ATX_SubInputStream_GetAvailable(ATX_InputStream* _self,
                                ATX_LargeSize*   available)
{
    ATX_SubInputStream* self = ATX_SELF(ATX_SubInputStream, ATX_InputStream);
    ATX_LargeSize       max_possible;
    ATX_LargeSize       parent_available = 0;
    ATX_Result          result;

    /* compute the max possible value */
    max_possible = self->size - self->offset;

    /* see how many bytes are available from the parent */
    result = ATX_InputStream_GetAvailable(self->parent,
                                          &parent_available);
    if (ATX_SUCCEEDED(result)) {
        *available = 
            parent_available < max_possible ? 
            parent_available : max_possible;
    } else {
        *available = 0;
    }
    
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_SubInputStream_GetInterface
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(ATX_SubInputStream)
    ATX_GET_INTERFACE_ACCEPT(ATX_SubInputStream, ATX_InputStream)
    ATX_GET_INTERFACE_ACCEPT(ATX_SubInputStream, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   ATX_InputStream interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(ATX_SubInputStream, ATX_InputStream)
    ATX_SubInputStream_Read,
    ATX_SubInputStream_Seek,
    ATX_SubInputStream_Tell,
    ATX_SubInputStream_GetSize,
    ATX_SubInputStream_GetAvailable
};

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE(ATX_SubInputStream, reference_count)

/*----------------------------------------------------------------------
|   ATX_MemoryStream
+---------------------------------------------------------------------*/
struct ATX_MemoryStream {
    /* intefaces */
    ATX_IMPLEMENTS(ATX_InputStream);
    ATX_IMPLEMENTS(ATX_OutputStream);
    ATX_IMPLEMENTS(ATX_Referenceable);

    /* members */
    ATX_Cardinal    reference_count;
    ATX_DataBuffer* buffer;
    ATX_Position    read_offset;
    ATX_Position    write_offset;
};

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(ATX_MemoryStream, ATX_InputStream)
ATX_DECLARE_INTERFACE_MAP(ATX_MemoryStream, ATX_OutputStream)
ATX_DECLARE_INTERFACE_MAP(ATX_MemoryStream, ATX_Referenceable)

/*----------------------------------------------------------------------
|   ATX_MemoryStream_Create
+---------------------------------------------------------------------*/
ATX_Result
ATX_MemoryStream_Create(ATX_Size size, ATX_MemoryStream** stream)
{ 
    return ATX_MemoryStream_CreateFromBuffer(NULL, size, stream);
}

/*----------------------------------------------------------------------
|   ATX_MemoryStream_CreateFromBuffer
+---------------------------------------------------------------------*/
ATX_Result
ATX_MemoryStream_CreateFromBuffer(ATX_Byte*          buffer, 
                                  ATX_Size           size, 
                                  ATX_MemoryStream** stream)
{ 
    ATX_Result result;

    /* allocate the object */
    *stream = ATX_AllocateZeroMemory(sizeof(ATX_MemoryStream));
    if (*stream == NULL) return ATX_ERROR_OUT_OF_MEMORY;

    /* construct the object */
    if (buffer == NULL) {
        result = ATX_DataBuffer_Create(size, &(*stream)->buffer);
    } else {
        result = ATX_DataBuffer_Create(0, &(*stream)->buffer);
    }
    if (ATX_FAILED(result)) {
        ATX_FreeMemory((void*)(*stream));
        return result;
    }
    
    /* set the buffer if needed */
    if (buffer != NULL) {
        result = ATX_DataBuffer_SetBuffer((*stream)->buffer, 
                                          buffer,
                                          size);
        if (ATX_SUCCEEDED(result)) {
            result = ATX_DataBuffer_SetDataSize((*stream)->buffer, size);
        }
        if (ATX_FAILED(result)) {
            ATX_DataBuffer_Destroy((*stream)->buffer);
            ATX_FreeMemory((void*)(*stream));
            return result;
        }
    }
    
    (*stream)->reference_count = 1;
    (*stream)->read_offset = 0;
    (*stream)->write_offset = 0;

    /* setup the interfaces */
    ATX_SET_INTERFACE(*stream, ATX_MemoryStream, ATX_InputStream);
    ATX_SET_INTERFACE(*stream, ATX_MemoryStream, ATX_OutputStream);
    ATX_SET_INTERFACE(*stream, ATX_MemoryStream, ATX_Referenceable);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_MemoryStream_AddReference
+---------------------------------------------------------------------*/
static ATX_Result
ATX_MemoryStream_AddReference(ATX_Referenceable* _self)
{
    ATX_MemoryStream* self = ATX_SELF(ATX_MemoryStream, ATX_Referenceable);
    ++self->reference_count;
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_MemoryStream_Release
+---------------------------------------------------------------------*/
static ATX_Result
ATX_MemoryStream_Release(ATX_Referenceable* _self)
{
    ATX_MemoryStream* self = ATX_SELF(ATX_MemoryStream, ATX_Referenceable);
    if (--self->reference_count == 0) {
        ATX_DataBuffer_Destroy(self->buffer);
        ATX_FreeMemory((void*)self);
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_MemoryStream_Destroy
+---------------------------------------------------------------------*/
ATX_Result
ATX_MemoryStream_Destroy(ATX_MemoryStream* self)
{
    return ATX_MemoryStream_Release(&ATX_BASE(self, ATX_Referenceable));
}

/*----------------------------------------------------------------------
|   ATX_MemoryStream_GetBuffer
+---------------------------------------------------------------------*/
ATX_Result 
ATX_MemoryStream_GetBuffer(ATX_MemoryStream*       self, 
                           const ATX_DataBuffer**  buffer)
{
    *buffer = self->buffer;
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_MemoryStream_GetInputStream
+---------------------------------------------------------------------*/
ATX_Result 
ATX_MemoryStream_GetInputStream(ATX_MemoryStream* self,
                                ATX_InputStream** stream)
{
    ++self->reference_count;
    *stream = &ATX_BASE(self, ATX_InputStream);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_MemoryStream_GetOutputStream
+---------------------------------------------------------------------*/
ATX_Result 
ATX_MemoryStream_GetOutputStream(ATX_MemoryStream*  self,
                                 ATX_OutputStream** stream)
{
    ++self->reference_count;
    *stream = &ATX_BASE(self, ATX_OutputStream);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_MemoryStream_Read
+---------------------------------------------------------------------*/
ATX_METHOD
ATX_MemoryStream_Read(ATX_InputStream* _self,
                      ATX_Any          buffer, 
                      ATX_Size         bytes_to_read, 
                      ATX_Size*        bytes_read)
{
    ATX_MemoryStream* self = ATX_SELF(ATX_MemoryStream, ATX_InputStream);
    ATX_Size          available;

    /* check for shortcut */
    if (bytes_to_read == 0) {
        if (bytes_read) *bytes_read = 0;
        return ATX_SUCCESS;
    }

    /* clip to what's available */
    available = ATX_DataBuffer_GetDataSize(self->buffer);
    if (self->read_offset+bytes_to_read > available) {
        bytes_to_read = (ATX_Size)(available-self->read_offset);
    }

    /* copy the data */
    if (bytes_to_read) {
        ATX_CopyMemory(buffer, (void*)(((char*)ATX_DataBuffer_UseData(self->buffer))+self->read_offset), bytes_to_read);
        self->read_offset += bytes_to_read;
    } 
    if (bytes_read) *bytes_read = bytes_to_read;

    return bytes_to_read?ATX_SUCCESS:ATX_ERROR_EOS; 
}

/*----------------------------------------------------------------------
|   ATX_MemoryStream_InputSeek
+---------------------------------------------------------------------*/
ATX_METHOD
ATX_MemoryStream_InputSeek(ATX_InputStream* _self, 
                           ATX_Position     offset)
{
    ATX_MemoryStream* self = ATX_SELF(ATX_MemoryStream, ATX_InputStream);
    if (offset > ATX_DataBuffer_GetDataSize(self->buffer)) {
        return ATX_ERROR_INVALID_PARAMETERS;
    } else {
        self->read_offset = offset;
        return ATX_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   ATX_MemoryStream_InputTell
+---------------------------------------------------------------------*/
ATX_METHOD
ATX_MemoryStream_InputTell(ATX_InputStream* _self, 
                           ATX_Position*    where)
{
    ATX_MemoryStream* self = ATX_SELF(ATX_MemoryStream, ATX_InputStream);
    if (where) *where = self->read_offset;
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_MemoryStream_GetSize
+---------------------------------------------------------------------*/
ATX_METHOD
ATX_MemoryStream_GetSize(ATX_InputStream* _self,
                         ATX_LargeSize*   size)
{
    ATX_MemoryStream* self = ATX_SELF(ATX_MemoryStream, ATX_InputStream);
    if (size) *size = ATX_DataBuffer_GetDataSize(self->buffer);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_MemoryStream_GetAvailable
+---------------------------------------------------------------------*/
ATX_METHOD
ATX_MemoryStream_GetAvailable(ATX_InputStream* _self,
                              ATX_LargeSize*   available)
{
    ATX_MemoryStream* self = ATX_SELF(ATX_MemoryStream, ATX_InputStream);
    *available = ATX_DataBuffer_GetDataSize(self->buffer)-self->read_offset; 
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_MemoryStream_Write
+---------------------------------------------------------------------*/
ATX_METHOD
ATX_MemoryStream_Write(ATX_OutputStream* _self,
                       ATX_AnyConst      data,
                       ATX_Size          bytes_to_write,
                       ATX_Size*         bytes_written)
{
    ATX_MemoryStream* self = ATX_SELF(ATX_MemoryStream, ATX_OutputStream);

    ATX_CHECK(ATX_DataBuffer_Reserve(self->buffer, (ATX_Size)(self->write_offset+bytes_to_write)));

    ATX_CopyMemory(ATX_DataBuffer_UseData(self->buffer)+self->write_offset, data, bytes_to_write);
    self->write_offset += bytes_to_write;
    if (self->write_offset > ATX_DataBuffer_GetDataSize(self->buffer)) {
        ATX_DataBuffer_SetDataSize(self->buffer, (ATX_Size)self->write_offset);
    }
    if (bytes_written) *bytes_written = bytes_to_write;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_MemoryStream_OutputSeek
+---------------------------------------------------------------------*/
ATX_METHOD
ATX_MemoryStream_OutputSeek(ATX_OutputStream* _self, 
                            ATX_Position      offset)
{
    ATX_MemoryStream* self = ATX_SELF(ATX_MemoryStream, ATX_OutputStream);
    if (offset <= ATX_DataBuffer_GetDataSize(self->buffer)) {
        self->write_offset = offset;
        return ATX_SUCCESS;
    } else {
        return ATX_ERROR_INVALID_PARAMETERS;
    }
}

/*----------------------------------------------------------------------
|   ATX_MemoryStream_OutputTell
+---------------------------------------------------------------------*/
ATX_METHOD
ATX_MemoryStream_OutputTell(ATX_OutputStream* _self, 
                            ATX_Position*     where)
{
    ATX_MemoryStream* self = ATX_SELF(ATX_MemoryStream, ATX_OutputStream);
    if (where) *where = self->write_offset;
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_MemoryStream_Flush
+---------------------------------------------------------------------*/
ATX_METHOD
ATX_MemoryStream_Flush(ATX_OutputStream* self)
{
    ATX_COMPILER_UNUSED(self);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_MemoryStream_GetInterface
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(ATX_MemoryStream)
    ATX_GET_INTERFACE_ACCEPT(ATX_MemoryStream, ATX_InputStream)
    ATX_GET_INTERFACE_ACCEPT(ATX_MemoryStream, ATX_OutputStream)
    ATX_GET_INTERFACE_ACCEPT(ATX_MemoryStream, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   ATX_InputStream interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(ATX_MemoryStream, ATX_InputStream)
    ATX_MemoryStream_Read,
    ATX_MemoryStream_InputSeek,
    ATX_MemoryStream_InputTell,
    ATX_MemoryStream_GetSize,
    ATX_MemoryStream_GetAvailable
};

/*----------------------------------------------------------------------
|   ATX_OutputStream interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(ATX_MemoryStream, ATX_OutputStream)
    ATX_MemoryStream_Write,
    ATX_MemoryStream_OutputSeek,
    ATX_MemoryStream_OutputTell,
    ATX_MemoryStream_Flush
};

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(ATX_MemoryStream, ATX_Referenceable)
    ATX_MemoryStream_AddReference,
    ATX_MemoryStream_Release
};
