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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "AtxFile.h"
#include "AtxResults.h"
#include "AtxTypes.h"
#include "AtxReferenceable.h"
#include "AtxDestroyable.h"

/*----------------------------------------------------------------------
|   interface stubs
+---------------------------------------------------------------------*/
ATX_Result 
ATX_File_Open(ATX_File* self, ATX_Flags mode)
{
    return ATX_INTERFACE(self)->Open(self, mode);
}

ATX_Result 
ATX_File_Close(ATX_File* self)
{
    return ATX_INTERFACE(self)->Close(self);
}

ATX_Result 
ATX_File_GetSize(ATX_File* self, ATX_LargeSize* size)
{
    return ATX_INTERFACE(self)->GetSize(self, size);
}

ATX_Result 
ATX_File_GetInputStream(ATX_File* self, ATX_InputStream** stream)
{
    return ATX_INTERFACE(self)->GetInputStream(self, stream);
}

ATX_Result 
ATX_File_GetOutputStream(ATX_File* self, ATX_OutputStream**  stream)
{
    return ATX_INTERFACE(self)->GetOutputStream(self, stream);
}

/*----------------------------------------------------------------------
|   ATX_File_Load
+---------------------------------------------------------------------*/
ATX_Result
ATX_File_Load(ATX_File* file, ATX_DataBuffer** buffer)
{
    ATX_InputStream* input = NULL;
    ATX_Result       result;

    /* get the input stream for the file */
    ATX_CHECK(ATX_File_GetInputStream(file, &input));

    /* read the stream */
    result = ATX_InputStream_Load(input, 0, buffer);

    /* release the stream */
    ATX_RELEASE_OBJECT(input);

    return result;
}

/*----------------------------------------------------------------------
|   ATX_File_Save
+---------------------------------------------------------------------*/
ATX_Result
ATX_File_Save(ATX_File* file, ATX_DataBuffer* buffer)
{
    ATX_OutputStream* output = NULL;
    ATX_Result        result;

    /* get the output stream for the file */
    ATX_CHECK(ATX_File_GetOutputStream(file, &output));

    /* write the stream */
    result = ATX_OutputStream_WriteFully(output, 
        ATX_DataBuffer_GetData(buffer),
        ATX_DataBuffer_GetDataSize(buffer));

    /* release the stream */
    ATX_RELEASE_OBJECT(output);

    return result;
}

/*----------------------------------------------------------------------
|   ATX_LoadFile
+---------------------------------------------------------------------*/
ATX_Result
ATX_LoadFile(ATX_CString filename, ATX_DataBuffer** buffer)
{
    ATX_File*  file;
    ATX_Result result;

    /* open the file */
    ATX_CHECK(ATX_File_Create(filename, &file));
    result = ATX_File_Open(file, ATX_FILE_OPEN_MODE_READ);
    if (ATX_FAILED(result)) {
        ATX_DESTROY_OBJECT(file);
        return result;
    }

    /* load the file */
    result = ATX_File_Load(file, buffer);

    /* close and destroy the file */
    ATX_File_Close(file);
    ATX_DESTROY_OBJECT(file);

    return result;
}

/*----------------------------------------------------------------------
|   ATX_SaveFile
+---------------------------------------------------------------------*/
ATX_Result
ATX_SaveFile(ATX_CString filename, ATX_DataBuffer* buffer)
{
    ATX_File*  file;
    ATX_Result result;

    /* open the file */
    ATX_CHECK(ATX_File_Create(filename, &file));
    result = ATX_File_Open(file, 
                           ATX_FILE_OPEN_MODE_CREATE   | 
                           ATX_FILE_OPEN_MODE_TRUNCATE | 
                           ATX_FILE_OPEN_MODE_WRITE);
    if (ATX_FAILED(result)) {
        ATX_DESTROY_OBJECT(file);
        return result;
    }

    /* load the file */
    result = ATX_File_Save(file, buffer);

    /* close and destroy the file */
    ATX_File_Close(file);
    ATX_DESTROY_OBJECT(file);

    return result;
}
