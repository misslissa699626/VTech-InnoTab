/*****************************************************************
|
|      File: AtxStdcFile.c
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
|       includes
+---------------------------------------------------------------------*/
#define _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE64
#define _FILE_OFFSET_BITS 64

#include "AtxConfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(ATX_CONFIG_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "AtxUtils.h"
#include "AtxStreams.h"
#include "AtxFile.h"
#include "AtxResults.h"
#include "AtxReferenceable.h"
#include "AtxDestroyable.h"

/*----------------------------------------------------------------------
|       types
+---------------------------------------------------------------------*/
typedef struct {
    ATX_Cardinal  reference_count;
    FILE*         file;
    ATX_LargeSize size;
    ATX_Position  position;
    ATX_String    name;
} StdcFileWrapper;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(ATX_InputStream);
    ATX_IMPLEMENTS(ATX_OutputStream);
    ATX_IMPLEMENTS(ATX_Referenceable);

    /* members */
    ATX_Cardinal     reference_count;
    StdcFileWrapper* file;
} StdcFileStream;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(ATX_File);
    ATX_IMPLEMENTS(ATX_Destroyable);

    /* members */
    ATX_String       name;
    ATX_Flags        mode;
    StdcFileWrapper* file;
} StdcFile;

/*----------------------------------------------------------------------
|       StdcFileWrapper_Create
+---------------------------------------------------------------------*/
static ATX_Result
StdcFileWrapper_Create(FILE*             file, 
                       ATX_String*       name,
                       StdcFileWrapper** wrapper)
{
    /* allocate a new object */
    (*wrapper) = ATX_AllocateZeroMemory(sizeof(StdcFileWrapper));
    if (*wrapper == NULL) return ATX_ERROR_OUT_OF_MEMORY;

    /* construct the object */
    (*wrapper)->file            = file;
    (*wrapper)->size            = 0;
    (*wrapper)->position        = 0;
    (*wrapper)->reference_count = 1;
    if (name) {
        ATX_String_Copy(&(*wrapper)->name, name);
    }
    
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
 |       StdcFileWrapper_UpdateSize
 +---------------------------------------------------------------------*/
static void
StdcFileWrapper_UpdateSize(StdcFileWrapper* self)
{
    if (self->file == NULL   || 
        self->file == stdin  || 
        self->file == stdout || 
        self->file == stderr) {
        return;
    }
        
#if defined(__SYMBIAN32__)
    /* Hack to get the filesize since PIPS stat() is broken. */
    ATX_fseek(self->file, 0L, SEEK_END); 
    self->size = ATX_ftell(stdc_file);
    ATX_fseek(self->file, self->position, SEEK_SET); /* return to where we were */
#endif
    {
        struct stat info;
        if (stat(ATX_CSTR(self->name), &info) == 0) {
            self->size = info.st_size;
        }
    }
}


/*----------------------------------------------------------------------
|       StdcFileWrapper_Destroy
+---------------------------------------------------------------------*/
static void
StdcFileWrapper_Destroy(StdcFileWrapper* self)
{
    if (self->file != NULL   &&
        self->file != stdin  &&
        self->file != stdout &&
        self->file != stderr) {
        fclose(self->file);
    }
    ATX_String_Destruct(&self->name);
    ATX_FreeMemory((void*)self);
}

/*----------------------------------------------------------------------
|       StdcFileWrapper_AddReference
+---------------------------------------------------------------------*/
static void
StdcFileWrapper_AddReference(StdcFileWrapper* self)
{
    ++self->reference_count;
}

/*----------------------------------------------------------------------
|       StdcFileWrapper_Release
+---------------------------------------------------------------------*/
static void
StdcFileWrapper_Release(StdcFileWrapper* self)
{
    if (self == NULL) return;
    if (--self->reference_count == 0) {
        StdcFileWrapper_Destroy(self);
    }
}

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(StdcFileStream, ATX_InputStream)
ATX_DECLARE_INTERFACE_MAP(StdcFileStream, ATX_OutputStream)
ATX_DECLARE_INTERFACE_MAP(StdcFileStream, ATX_Referenceable)

/*----------------------------------------------------------------------
|       StdcFileStream_Create
+---------------------------------------------------------------------*/
static ATX_Result
StdcFileStream_Create(StdcFileWrapper* file, StdcFileStream** stream)
{
    /* create a new object */
    (*stream) = (StdcFileStream*)ATX_AllocateMemory(sizeof(StdcFileStream));
    if (*stream == NULL) return ATX_ERROR_OUT_OF_MEMORY;

    /* construct the object */
    (*stream)->reference_count = 1;
    (*stream)->file = file;

    /* keep a reference */
    StdcFileWrapper_AddReference(file);

    /* setup interfaces */
    ATX_SET_INTERFACE((*stream), StdcFileStream, ATX_InputStream);
    ATX_SET_INTERFACE((*stream), StdcFileStream, ATX_OutputStream);
    ATX_SET_INTERFACE((*stream), StdcFileStream, ATX_Referenceable);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|       StdcFileStream_Destroy
+---------------------------------------------------------------------*/
static ATX_Result
StdcFileStream_Destroy(StdcFileStream* self)
{
    StdcFileWrapper_Release(self->file);
    ATX_FreeMemory((void*)self);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|       StdcFileStream_Seek
+---------------------------------------------------------------------*/
ATX_METHOD
StdcFileStream_Seek(StdcFileStream* self, ATX_Position where)
{
    if (ATX_fseek(self->file->file, where, SEEK_SET) == 0) {
        self->file->position = where;
        return ATX_SUCCESS;
    } else {
        return ATX_FAILURE;
    }
}

/*----------------------------------------------------------------------
|       StdcFileStream_Tell
+---------------------------------------------------------------------*/
ATX_METHOD
StdcFileStream_Tell(StdcFileStream* self, ATX_Position* where)
{
    if (where) *where = self->file->position;
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|       StdcFileStream_Flush
+---------------------------------------------------------------------*/
ATX_METHOD
StdcFileStream_Flush(StdcFileStream* self)
{
    fflush(self->file->file);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|       StdcFileInputStream_Create
+---------------------------------------------------------------------*/
static ATX_Result
StdcFileInputStream_Create(StdcFileWrapper*  file, 
                           ATX_InputStream** stream)
{
    StdcFileStream* file_stream = NULL;
    ATX_Result      result;

    /* create the object */
    result = StdcFileStream_Create(file, &file_stream);
    if (ATX_FAILED(result)) {
        *stream = NULL;
        return result;
    }

    /* select the ATX_InputStream interface */
    *stream = &ATX_BASE(file_stream, ATX_InputStream);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|       StdcFileInputStream_Read
+---------------------------------------------------------------------*/
ATX_METHOD
StdcFileInputStream_Read(ATX_InputStream* _self,
                         ATX_Any          buffer, 
                         ATX_Size         bytes_to_read, 
                         ATX_Size*        bytes_read)
{
    StdcFileStream* self = ATX_SELF(StdcFileStream, ATX_InputStream);
    size_t          nb_read;

    /* shortcut */
    if (bytes_to_read == 0) {
        if (bytes_read) *bytes_read = 0;
        return ATX_SUCCESS;
    }
    
    /* if we predict an EOF condition, clear the EOF flag, in case the file has grown */
    if (self->file->position+bytes_to_read > self->file->size) {
        StdcFileWrapper_UpdateSize(self->file);
        clearerr(self->file->file);
    }
    
    /* read from the file */
    nb_read = fread(buffer, 1, (size_t)bytes_to_read, self->file->file);
    if (nb_read > 0 || bytes_to_read == 0) {
        if (bytes_read) *bytes_read = (ATX_Size)nb_read;
        self->file->position += nb_read;
        return ATX_SUCCESS;
    } else {
        if (bytes_read) *bytes_read = 0;
        if (nb_read == 0 || feof(self->file->file) != 0) {
            return ATX_ERROR_EOS;
        } else {
            return ATX_FAILURE;
        }
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|       StdcFileInputStream_Seek
+---------------------------------------------------------------------*/
ATX_METHOD
StdcFileInputStream_Seek(ATX_InputStream* _self, 
                         ATX_Position     where)
{
    return StdcFileStream_Seek(ATX_SELF(StdcFileStream, ATX_InputStream), where);
}

/*----------------------------------------------------------------------
|       StdcFileInputStream_Tell
+---------------------------------------------------------------------*/
ATX_METHOD
StdcFileInputStream_Tell(ATX_InputStream* _self, 
                         ATX_Position*    where)
{
    return StdcFileStream_Tell(ATX_SELF(StdcFileStream, ATX_InputStream), where);
}

/*----------------------------------------------------------------------
|       StdcFileInputStream_GetSize
+---------------------------------------------------------------------*/
ATX_METHOD
StdcFileInputStream_GetSize(ATX_InputStream* _self, 
                            ATX_LargeSize*   size)
{
    StdcFileStream* self = ATX_SELF(StdcFileStream, ATX_InputStream);
    StdcFileWrapper_UpdateSize(self->file);
    *size = self->file->size;
    
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|       StdcFileInputStream_GetAvailable
+---------------------------------------------------------------------*/
ATX_METHOD
StdcFileInputStream_GetAvailable(ATX_InputStream* _self, 
                                 ATX_LargeSize*   size)
{
    StdcFileStream* self = ATX_SELF(StdcFileStream, ATX_InputStream);
    
    StdcFileWrapper_UpdateSize(self->file);
    if (self->file->position > self->file->size) {
        *size = self->file->size - self->file->position;
    } else {
        *size = 0;
    }
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|       StdcFileOutputStream_Create
+---------------------------------------------------------------------*/
static ATX_Result
StdcFileOutputStream_Create(StdcFileWrapper*   file, 
                            ATX_OutputStream** stream)
{
    StdcFileStream* file_stream = NULL;
    ATX_Result      result;

    /* create the object */
    result = StdcFileStream_Create(file, &file_stream);
    if (ATX_FAILED(result)) {
        *stream = NULL;
        return result;
    }

    /* select the ATX_InputStream interface */
    *stream = &ATX_BASE(file_stream, ATX_OutputStream);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|       StdcFileOutputStream_Write
+---------------------------------------------------------------------*/
ATX_METHOD
StdcFileOutputStream_Write(ATX_OutputStream* _self,
                           ATX_AnyConst      buffer, 
                           ATX_Size          bytes_to_write, 
                           ATX_Size*         bytes_written)
{
    StdcFileStream* self = ATX_SELF(StdcFileStream, ATX_OutputStream);
    size_t          nb_written;

    nb_written = fwrite(buffer, 1, (size_t)bytes_to_write, self->file->file);
    if (nb_written > 0 || bytes_to_write == 0) {
        if (bytes_written) *bytes_written = (ATX_Size)nb_written;
        self->file->position += nb_written;
        return ATX_SUCCESS;
    } else {
        if (bytes_written) *bytes_written = 0;
        return ATX_FAILURE;
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|       StdcFileOutputStream_Seek
+---------------------------------------------------------------------*/
ATX_METHOD
StdcFileOutputStream_Seek(ATX_OutputStream* _self, 
                          ATX_Position      where)
{
    return StdcFileStream_Seek(ATX_SELF(StdcFileStream, ATX_OutputStream), where);
}

/*----------------------------------------------------------------------
|       StdcFileOutputStream_Tell
+---------------------------------------------------------------------*/
ATX_METHOD
StdcFileOutputStream_Tell(ATX_OutputStream* _self, 
                          ATX_Position*     where)
{
    return StdcFileStream_Tell(ATX_SELF(StdcFileStream, ATX_OutputStream), where);
}

/*----------------------------------------------------------------------
|       StdcFileOutputStream_Flush
+---------------------------------------------------------------------*/
ATX_METHOD
StdcFileOutputStream_Flush(ATX_OutputStream* _self)
{
    return StdcFileStream_Flush(ATX_SELF(StdcFileStream, ATX_OutputStream));
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(StdcFileStream)
    ATX_GET_INTERFACE_ACCEPT(StdcFileStream, ATX_InputStream)
    ATX_GET_INTERFACE_ACCEPT(StdcFileStream, ATX_OutputStream)
    ATX_GET_INTERFACE_ACCEPT(StdcFileStream, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|       ATX_InputStream interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(StdcFileStream, ATX_InputStream)
    StdcFileInputStream_Read,
    StdcFileInputStream_Seek,
    StdcFileInputStream_Tell,
    StdcFileInputStream_GetSize,
    StdcFileInputStream_GetAvailable
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|       ATX_OutputStream interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(StdcFileStream, ATX_OutputStream)
    StdcFileOutputStream_Write,
    StdcFileOutputStream_Seek,
    StdcFileOutputStream_Tell,
    StdcFileOutputStream_Flush
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|       ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE(StdcFileStream, reference_count)

/*----------------------------------------------------------------------
|       forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(StdcFile, ATX_File)
ATX_DECLARE_INTERFACE_MAP(StdcFile, ATX_Destroyable)

/*----------------------------------------------------------------------
|       ATX_File_Create
+---------------------------------------------------------------------*/
ATX_Result
ATX_File_Create(const char* filename, ATX_File** object)
{
    StdcFile* file;

    /* allocate a new object */
    file = (StdcFile*)ATX_AllocateZeroMemory(sizeof(StdcFile));
    if (file == NULL) {
        *object = NULL;
        return ATX_ERROR_OUT_OF_MEMORY;
    }

    /* construct the object */
    file->name = ATX_String_Create(filename);

    /* setup the interfaces */
    ATX_SET_INTERFACE(file, StdcFile, ATX_File);
    ATX_SET_INTERFACE(file, StdcFile, ATX_Destroyable);
    *object = &ATX_BASE(file, ATX_File);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|       StdcFile_Destroy
+---------------------------------------------------------------------*/
ATX_METHOD
StdcFile_Destroy(ATX_Destroyable* _self)
{
    StdcFile* self = ATX_SELF(StdcFile, ATX_Destroyable);

    /* release the resources */
    ATX_String_Destruct(&self->name);
    StdcFileWrapper_Release(self->file);

    /* free the memory */
    ATX_FreeMemory((void*)self);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|       StdcFile_Open
+---------------------------------------------------------------------*/
ATX_METHOD
StdcFile_Open(ATX_File* _self, ATX_Flags mode)
{ 
    StdcFile* self = ATX_SELF(StdcFile, ATX_File);
    FILE*     stdc_file;

    /* decide wheter this is a file or stdin/stdout/stderr */
    if (ATX_String_Equals(&self->name, 
                          ATX_FILE_STANDARD_INPUT, 
                          ATX_FALSE)) {
        stdc_file = stdin;
    } else if (ATX_String_Equals(&self->name, 
                                 ATX_FILE_STANDARD_OUTPUT, 
                                 ATX_FALSE)) {
        stdc_file = stdout;
    } else if (ATX_String_Equals(&self->name, 
                                 ATX_FILE_STANDARD_ERROR,
                                 ATX_FALSE)) {
        stdc_file = stderr;
    } else {
        const char* fmode = "";

        /* compute mode */
        if (mode & ATX_FILE_OPEN_MODE_WRITE) {
            if (mode & ATX_FILE_OPEN_MODE_CREATE) {
                if (mode & ATX_FILE_OPEN_MODE_TRUNCATE) {
                    /* write, read, create, truncate */
                    fmode = "w+b";
                } else {
                    /* write, read, create */
                    fmode = "a+b";
                }
            } else {
                if (mode & ATX_FILE_OPEN_MODE_TRUNCATE) {
                    /* write, read, truncate */
                    fmode = "w+b";
                } else {
                    /* write, read */
                    fmode = "r+b";
                }
            }
        } else {
            /* read only */
            fmode = "rb";
        }

        /* try to open the file */
        stdc_file = fopen(ATX_CSTR(self->name), fmode);
        if (stdc_file == NULL) {
            switch (errno) {
              case EACCES:
                return ATX_ERROR_ACCESS_DENIED;

              case ENOENT:
                return ATX_ERROR_NO_SUCH_FILE;

              default:
                return ATX_ERROR_ERRNO(errno);
            }
        }
    }

    /* set the buffered/unbuffered option */
    if (mode & ATX_FILE_OPEN_MODE_UNBUFFERED) {
        setvbuf(stdc_file, NULL, _IONBF, 0);
    }

    /* remember the mode */
    self->mode = mode;

    /* create a wrapper */
    return StdcFileWrapper_Create(stdc_file, &self->name, &self->file);
}

/*----------------------------------------------------------------------
|       StdcFile_Close
+---------------------------------------------------------------------*/
static ATX_Result
StdcFile_Close(ATX_File* _self)
{
    StdcFile* self = ATX_SELF(StdcFile, ATX_File);

    /* release the resources and reset */
    StdcFileWrapper_Release(self->file);
    self->file = NULL;
    self->mode = 0;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|       StdcFile_GetSize
+---------------------------------------------------------------------*/
ATX_METHOD
StdcFile_GetSize(ATX_File* _self, ATX_LargeSize* size)
{
    StdcFile* self = ATX_SELF(StdcFile, ATX_File);

    /* check that the file is open */
    if (self->file == NULL) return ATX_ERROR_FILE_NOT_OPEN;

    /* return the size */
    StdcFileWrapper_UpdateSize(self->file);
    *size = self->file->size;
    
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|       StdcFile_GetInputStream
+---------------------------------------------------------------------*/
ATX_METHOD
StdcFile_GetInputStream(ATX_File*          _self, 
                        ATX_InputStream**  stream)
{
    StdcFile* self = ATX_SELF(StdcFile, ATX_File);

    /* check that the file is open */
    if (self->file == NULL) return ATX_ERROR_FILE_NOT_OPEN;

    /* check that the mode is compatible */
    if (!(self->mode & ATX_FILE_OPEN_MODE_READ)) {
        return ATX_ERROR_FILE_NOT_READABLE;
    }

    return StdcFileInputStream_Create(self->file, stream);
}

/*----------------------------------------------------------------------
|       StdcFile_GetOutputStream
+---------------------------------------------------------------------*/
ATX_METHOD
StdcFile_GetOutputStream(ATX_File*          _self, 
                         ATX_OutputStream** stream)
{
    StdcFile* self = ATX_SELF(StdcFile, ATX_File);

    /* check that the file is open */
    if (self->file == NULL) return ATX_ERROR_FILE_NOT_OPEN;

    /* check that the mode is compatible */
    if (!(self->mode & ATX_FILE_OPEN_MODE_WRITE)) {
        return ATX_ERROR_FILE_NOT_WRITABLE;
    }

    return StdcFileOutputStream_Create(self->file, stream);
}

/*----------------------------------------------------------------------
|   StdcFile_GetInterface
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(StdcFile)
    ATX_GET_INTERFACE_ACCEPT(StdcFile, ATX_File)
    ATX_GET_INTERFACE_ACCEPT(StdcFile, ATX_Destroyable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|       ATX_File interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(StdcFile, ATX_File)
    StdcFile_Open,
    StdcFile_Close,
    StdcFile_GetSize,
    StdcFile_GetInputStream,
    StdcFile_GetOutputStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|       ATX_Destroyable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_DESTROYABLE_INTERFACE(StdcFile)
