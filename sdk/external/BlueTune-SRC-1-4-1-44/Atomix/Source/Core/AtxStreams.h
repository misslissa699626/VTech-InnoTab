/*****************************************************************
|
|   Atomix - Byte Streams
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

#ifndef _ATX_STREAMS_H_
#define _ATX_STREAMS_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "AtxInterfaces.h"
#include "AtxTypes.h"
#include "AtxResults.h"
#include "AtxDataBuffer.h"
#include "AtxString.h"

/*----------------------------------------------------------------------
|   error codes
+---------------------------------------------------------------------*/
#define ATX_ERROR_EOS (ATX_ERROR_BASE_BYTE_STREAM - 0)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define ATX_INPUT_STREAM_PROPERTY_SEEK_SPEED   "SeekSpeed"
#define ATX_INPUT_STREAM_SEEK_SPEED_NO_SEEK    0 /** cannot seek  */
#define ATX_INPUT_STREAM_SEEK_SPEED_SLOW       1 /** slow         */
#define ATX_INPUT_STREAM_SEEK_SPEED_MEDIUM     2 /** medium speed */
#define ATX_INPUT_STREAM_SEEK_SPEED_FAST       3 /** fast speed   */

/*----------------------------------------------------------------------
|   ATX_InputStream
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(ATX_InputStream)
ATX_BEGIN_INTERFACE_DEFINITION(ATX_InputStream)
    ATX_Result (*Read)(ATX_InputStream* self, 
                       ATX_Any          buffer,
                       ATX_Size         bytes_to_read,
                       ATX_Size*        bytes_read);
    ATX_Result (*Seek)(ATX_InputStream* self, ATX_Position  offset);
    ATX_Result (*Tell)(ATX_InputStream* self, ATX_Position* offset);
    ATX_Result (*GetSize)(ATX_InputStream* self, ATX_LargeSize* size);
    ATX_Result (*GetAvailable)(ATX_InputStream* self, 
                               ATX_LargeSize*   available);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   ATX_OutputStream
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(ATX_OutputStream)
ATX_BEGIN_INTERFACE_DEFINITION(ATX_OutputStream)
    ATX_Result (*Write)(ATX_OutputStream* self,
                        ATX_AnyConst      buffer,
                        ATX_Size          bytes_to_write,
                        ATX_Size*         bytes_written);
    ATX_Result (*Seek)(ATX_OutputStream* self, ATX_Position  offset);
    ATX_Result (*Tell)(ATX_OutputStream* self, ATX_Position* offset);
    ATX_Result (*Flush)(ATX_OutputStream* self);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   base class implementations
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

ATX_Result ATX_OutputStream_WriteFully(ATX_OutputStream* stream,
                                       ATX_AnyConst      buffer,
                                       ATX_Size          bytes_to_write);
ATX_Result ATX_OutputStream_WriteString(ATX_OutputStream* stream,
                                        ATX_CString       string);
ATX_Result ATX_OutputStream_WriteLine(ATX_OutputStream* stream,
                                      ATX_CString       line);
ATX_Result ATX_InputStream_ReadLine(ATX_InputStream* stream,
                                    char*            line,
                                    ATX_Size         line_size,
                                    ATX_Size*        chars_read);

ATX_Result ATX_InputStream_ReadLineString(ATX_InputStream* stream,
                                          ATX_String*      string,
                                          ATX_Size         max_length);

ATX_Result ATX_InputStream_ReadUI64(ATX_InputStream* stream,
                                    ATX_UInt64*      value);

ATX_Result ATX_InputStream_ReadUI32(ATX_InputStream* stream,
                                    ATX_UInt32*      value);

ATX_Result ATX_InputStream_ReadUI16(ATX_InputStream* stream,
                                    ATX_UInt16*      value);

ATX_Result ATX_InputStream_ReadFully(ATX_InputStream* stream,
                                     ATX_Any          buffer,
                                     ATX_Size         bytes_to_read);

ATX_Result ATX_InputStream_Skip(ATX_InputStream* stream,
                                ATX_Size         count);

ATX_Result ATX_InputStream_Load(ATX_InputStream* stream, 
                                ATX_Size         max_read, /* = 0 if no limit */
                                ATX_DataBuffer** buffer);
#ifdef __cplusplus
}
#endif /* __cplusplus */

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define ATX_InputStream_Read(object, buffer, bytes_to_read, bytes_read)\
ATX_INTERFACE(object)->Read(object,                                    \
                            buffer,                                    \
                            bytes_to_read,                             \
                            bytes_read)

#define ATX_InputStream_Seek(object, where) \
ATX_INTERFACE(object)->Seek(object, where)

#define ATX_InputStream_Tell(object, where) \
ATX_INTERFACE(object)->Tell(object, where)

#define ATX_InputStream_GetSize(object, size) \
ATX_INTERFACE(object)->GetSize(object, size)

#define ATX_InputStream_GetAvailable(object, available) \
ATX_INTERFACE(object)->GetAvailable(object, available)

#define ATX_OutputStream_Write(object, buffer, bytes_to_write, bytes_written) \
ATX_INTERFACE(object)->Write(object,                                          \
                             buffer,                                          \
                             bytes_to_write,                                  \
                             bytes_written)       

#define ATX_OutputStream_Seek(object, where) \
ATX_INTERFACE(object)->Seek(object, where)

#define ATX_OutputStream_Tell(object, where) \
ATX_INTERFACE(object)->Tell(object, where)

#define ATX_OutputStream_Flush(object) \
ATX_INTERFACE(object)->Flush(object)

/*----------------------------------------------------------------------
|   ATX_StreamTransformer
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(ATX_StreamTransformer)
ATX_BEGIN_INTERFACE_DEFINITION(ATX_StreamTransformer)
    ATX_Result (*Transform)(ATX_StreamTransformer* self,
                            ATX_AnyConst           buffer,
                            ATX_Size               size);
ATX_END_INTERFACE_DEFINITION

#define ATX_StreamTransformer_Transform(object, buffer, size) \
ATX_INTERFACE(object)->Transform(object, buffer, size)

/*----------------------------------------------------------------------
|   ATX_MemoryStream
+---------------------------------------------------------------------*/
typedef struct ATX_MemoryStream ATX_MemoryStream;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

ATX_Result 
ATX_MemoryStream_Create(ATX_Size size, ATX_MemoryStream** stream);

ATX_Result 
ATX_MemoryStream_CreateFromBuffer(ATX_Byte*          buffer,
                                  ATX_Size           size,
                                  ATX_MemoryStream** stream);

ATX_Result 
ATX_MemoryStream_Destroy(ATX_MemoryStream* self);

ATX_Result 
ATX_MemoryStream_GetBuffer(ATX_MemoryStream*      self,
                           const ATX_DataBuffer** buffer);

ATX_Result 
ATX_MemoryStream_GetInputStream(ATX_MemoryStream* self,
                                ATX_InputStream** stream);

ATX_Result 
ATX_MemoryStream_GetOutputStream(ATX_MemoryStream* self,
                                 ATX_OutputStream** stream);
#ifdef __cplusplus
}
#endif /* __cplusplus */

/*----------------------------------------------------------------------
|   functions
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

ATX_Result ATX_SubInputStream_Create(ATX_InputStream*       parent,
                                     ATX_Position           offset,
                                     ATX_LargeSize          size,
                                     ATX_StreamTransformer* transformer,
                                     ATX_InputStream**      stream);

ATX_Result ATX_SubOutputStream_Create(ATX_OutputStream*      parent,
                                      ATX_Position           offset,
                                      ATX_LargeSize          size,
                                      ATX_StreamTransformer* transformer,
                                      ATX_OutputStream**     stream);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ATX_STREAMS_H_ */

