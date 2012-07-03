/*****************************************************************
|
|      Atomix Tests - Files
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
#include "Atomix.h"

/*----------------------------------------------------------------------
|       macros
+---------------------------------------------------------------------*/
#define SHOULD_SUCCEED(r)                                   \
    do {                                                    \
        if (ATX_FAILED(r)) {                                \
            ATX_ConsoleOutputF("failed line %d (%d)\n", __LINE__, r);\
            exit(1);                                        \
        }                                                   \
    } while(0)                                         

#define SHOULD_FAIL(r)                                                  \
    do {                                                                \
        if (ATX_SUCCEEDED(r)) {                                         \
            ATX_ConsoleOutputF("should have failed line %d (%d)\n", __LINE__, r);\
            exit(1);                                                    \
        }                                                               \
    } while(0)                                  

#define SHOULD_EQUAL_I(a, b)                                           \
    do {                                                               \
        if ((a) != (b)) {                                              \
            ATX_ConsoleOutputF("got %d, expected %d line %d\n", (int)a, (int)b, __LINE__);\
            exit(1);                                                   \
        }                                                              \
    } while(0)                                                            

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int 
main(int argc, char** argv)
{
    ATX_File*         file;
    ATX_File*         file2;
    ATX_InputStream*  input;
    ATX_InputStream*  input2;
    ATX_OutputStream* output;
    ATX_LargeSize     size;
    ATX_Position      position;
    unsigned char     buffer[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};  
    const char*       filename = "pi.\xCF\x80.test";
    unsigned int      i;
    ATX_TimeInterval  wait = {2,0};

    if (argc > 1) {
        filename = argv[1];
    }

    SHOULD_SUCCEED(ATX_File_Create(filename, &file));
    ATX_File_Open(file, ATX_FILE_OPEN_MODE_CREATE | ATX_FILE_OPEN_MODE_WRITE | ATX_FILE_OPEN_MODE_READ | ATX_FILE_OPEN_MODE_TRUNCATE);
    SHOULD_SUCCEED(ATX_File_GetSize(file, &size));
    SHOULD_EQUAL_I(size, 0);
    SHOULD_SUCCEED(ATX_File_GetOutputStream(file, &output));
    SHOULD_SUCCEED(ATX_File_GetInputStream(file, &input));
    SHOULD_SUCCEED(ATX_OutputStream_Tell(output, &position));
    SHOULD_EQUAL_I(position, 0);
    SHOULD_SUCCEED(ATX_InputStream_Tell(input, &position));
    SHOULD_EQUAL_I(position, 0);
    SHOULD_SUCCEED(ATX_OutputStream_WriteFully(output, buffer, 16));
    ATX_OutputStream_Flush(output);
    ATX_System_Sleep(&wait);
    SHOULD_SUCCEED(ATX_File_GetSize(file, &size));
    SHOULD_EQUAL_I(size, 16);
    SHOULD_SUCCEED(ATX_OutputStream_Tell(output, &position));
    SHOULD_SUCCEED(ATX_InputStream_GetSize(input, &size));
    SHOULD_EQUAL_I(size, 16);
    SHOULD_EQUAL_I(position, 16);
    SHOULD_SUCCEED(ATX_InputStream_Tell(input, &position));
    SHOULD_EQUAL_I(position, 16);
    SHOULD_SUCCEED(ATX_OutputStream_Seek(output, 8));
    SHOULD_SUCCEED(ATX_OutputStream_Tell(output, &position));
    SHOULD_EQUAL_I(position, 8);

    SHOULD_SUCCEED(ATX_File_Create(filename, &file2));
    ATX_File_Open(file2, ATX_FILE_OPEN_MODE_READ);
    SHOULD_SUCCEED(ATX_File_GetSize(file2, &size));
    SHOULD_EQUAL_I(size, 16);
    SHOULD_SUCCEED(ATX_File_GetInputStream(file2, &input2));
    SHOULD_SUCCEED(ATX_InputStream_GetSize(input2, &size));
    SHOULD_EQUAL_I(size, 16);
    SHOULD_SUCCEED(ATX_InputStream_Tell(input2, &position));
    SHOULD_EQUAL_I(position, 0);
    SHOULD_SUCCEED(ATX_InputStream_Seek(input2, 8));
    SHOULD_SUCCEED(ATX_InputStream_Tell(input2, &position));
    SHOULD_EQUAL_I(position, 8);

    SHOULD_SUCCEED(ATX_OutputStream_WriteFully(output, buffer, 16));
    ATX_OutputStream_Flush(output);
    ATX_System_Sleep(&wait);
    SHOULD_SUCCEED(ATX_File_GetSize(file, &size));
    SHOULD_EQUAL_I(size, 24);
    SHOULD_SUCCEED(ATX_OutputStream_Tell(output, &position));
    SHOULD_EQUAL_I(position, 24);
    SHOULD_SUCCEED(ATX_InputStream_Tell(input, &position));
    SHOULD_EQUAL_I(position, 24);

    SHOULD_SUCCEED(ATX_InputStream_GetSize(input2, &size));
    SHOULD_EQUAL_I(size, 24);
    SHOULD_SUCCEED(ATX_InputStream_Seek(input2, 20));
    SHOULD_SUCCEED(ATX_InputStream_Read(input2, buffer, 4, NULL));

    ATX_File_Close(file);
    ATX_RELEASE_OBJECT(input);
    ATX_RELEASE_OBJECT(output);
    ATX_ConsoleOutput("reading look (to check if the size is updated when some other process writes to the file. Please have some external app append to the file).\n");
    for (i=0; i<100; i++) {
        ATX_Size bytes_read = 0;
        ATX_Result result;
        ATX_TimeInterval sleep_time = {3,0};
        ATX_ConsoleOutputF("[%02d] reading 16 bytes...\n", i);
        result = ATX_InputStream_Read(input2, buffer, 16, &bytes_read);
        ATX_ConsoleOutputF("result = %d, got %d bytes\n", result, bytes_read);
        ATX_System_Sleep(&sleep_time);
    }
        
    return 0;
}

