/*****************************************************************
|
|   Atomix - File Storage
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

#ifndef _ATX_FILE_H_
#define _ATX_FILE_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "AtxTypes.h"
#include "AtxStreams.h"
#include "AtxDataBuffer.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define ATX_FILE_OPEN_MODE_READ       0x01
#define ATX_FILE_OPEN_MODE_WRITE      0x02
#define ATX_FILE_OPEN_MODE_CREATE     0x04
#define ATX_FILE_OPEN_MODE_TRUNCATE   0x08
#define ATX_FILE_OPEN_MODE_APPEND     0x10
#define ATX_FILE_OPEN_MODE_UNBUFFERED 0x20

#define ATX_ERROR_NO_SUCH_FILE       (ATX_ERROR_BASE_FILE - 0)
#define ATX_ERROR_FILE_NOT_OPEN      (ATX_ERROR_BASE_FILE - 1)
#define ATX_ERROR_FILE_BUSY          (ATX_ERROR_BASE_FILE - 2)
#define ATX_ERROR_FILE_ALREADY_OPEN  (ATX_ERROR_BASE_FILE - 3)
#define ATX_ERROR_FILE_NOT_READABLE  (ATX_ERROR_BASE_FILE - 4)
#define ATX_ERROR_FILE_NOT_WRITABLE  (ATX_ERROR_BASE_FILE - 5)

#define ATX_FILE_STANDARD_INPUT  "@STDIN"
#define ATX_FILE_STANDARD_OUTPUT "@STDOUT"
#define ATX_FILE_STANDARD_ERROR  "@STDERR"

/*----------------------------------------------------------------------
|   ATX_File
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(ATX_File)
ATX_BEGIN_INTERFACE_DEFINITION(ATX_File)
    ATX_Result (*Open)(ATX_File* self, ATX_Flags mode);
    ATX_Result (*Close)(ATX_File* self);
    ATX_Result (*GetSize)(ATX_File* self, ATX_LargeSize* size);
    ATX_Result (*GetInputStream)(ATX_File* self, ATX_InputStream** stream);
    ATX_Result (*GetOutputStream)(ATX_File* self, ATX_OutputStream**  stream);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   interface stubs
+---------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif

ATX_Result ATX_File_Open(ATX_File* self, ATX_Flags mode);
ATX_Result ATX_File_Close(ATX_File* self);
ATX_Result ATX_File_GetSize(ATX_File* self, ATX_LargeSize* size);
ATX_Result ATX_File_GetInputStream(ATX_File* self, ATX_InputStream** stream);
ATX_Result ATX_File_GetOutputStream(ATX_File* self, ATX_OutputStream**  stream);

#if defined(__cplusplus)
}
#endif

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern ATX_Result ATX_File_Create(ATX_CString name, ATX_File** file);
extern ATX_Result ATX_File_Load(ATX_File* file, ATX_DataBuffer** buffer);
extern ATX_Result ATX_File_Save(ATX_File* file, ATX_DataBuffer* buffer);

/* helper functions */
extern ATX_Result ATX_LoadFile(ATX_CString filename, ATX_DataBuffer** buffer);
extern ATX_Result ATX_SaveFile(ATX_CString filename, ATX_DataBuffer* buffer);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _ATX_FILE_H_ */


