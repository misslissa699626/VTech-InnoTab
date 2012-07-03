/*****************************************************************
|
|   BlueTune - Network Stream Test
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>

#include "Atomix.h"
#include "BltNetworkStream.h"

/*----------------------------------------------------------------------
|    CHECK
+---------------------------------------------------------------------*/
static void CHECK(int x)                             
{               
    static unsigned long counter = 0;
    ++counter;
    if (!(x)) {                                      
        fprintf(stderr, "FAILED line %d", __LINE__); 
        abort();                                     
    }                                                
}

/*----------------------------------------------------------------------
|    Test
+---------------------------------------------------------------------*/
static void
Test(int buffer_size, int source_size)
{
    ATX_MemoryStream* memory;
    ATX_InputStream*  source;
    ATX_OutputStream* source_buffer;
    ATX_InputStream*  stream;
    ATX_Offset        offset = 0;
    int               i; 
    unsigned char     scratch[4096];
    ATX_Boolean       expect_eos = ATX_FALSE;
    ATX_Result        result;

    /* create and setup the source */
    ATX_MemoryStream_Create(source_size, &memory);
    ATX_MemoryStream_GetOutputStream(memory, &source_buffer);
    ATX_MemoryStream_GetInputStream(memory, &source);
    for (i=0; i<source_size; i++) {
        unsigned char x = (unsigned char)i;
        ATX_OutputStream_Write(source_buffer, &x, 1, NULL);        
    }
    ATX_OutputStream_Seek(source_buffer, 0);
    ATX_RELEASE_OBJECT(source_buffer);
    ATX_MemoryStream_Destroy(memory);

    /* create the stream */
    BLT_NetworkStream_Create(buffer_size, source, &stream);
    offset = 0;
    for (;;) {
        ATX_Size bytes_read = 0;
        ATX_Size chunk;
        if ((ATX_System_GetRandomInteger()%7) == 0) {
            chunk = ATX_System_GetRandomInteger()%(10+source_size);
        } else {
            chunk = ATX_System_GetRandomInteger()%7;
        }
        if ((ATX_System_GetRandomInteger()%5) == 0) {
            ATX_Position position = ATX_System_GetRandomInteger()%(10+source_size);
            ATX_Position new_position;
            ATX_Result   result = ATX_InputStream_Seek(stream, position);
            if (ATX_SUCCEEDED(result)) {
                ATX_InputStream_Tell(stream, &new_position);
                CHECK(new_position == position);
                expect_eos = ATX_FALSE;
            } else {
                result = ATX_InputStream_Seek(stream, offset);
                CHECK(result == ATX_SUCCESS);
                ATX_InputStream_Tell(stream, &new_position);
                CHECK(new_position == (ATX_Position)offset);
            }
            offset = new_position;
        }
        ATX_SetMemory(scratch, 0, chunk);
        result = ATX_InputStream_Read(stream, scratch, chunk, &bytes_read);
        if (ATX_FAILED(result)) {
            CHECK(result == ATX_ERROR_EOS);
            CHECK(offset == source_size);
            break;
        }
        CHECK(chunk == 0 || expect_eos == ATX_FALSE);
        CHECK(bytes_read <= chunk);
        if (bytes_read != chunk) {
            expect_eos = ATX_TRUE;
        }
        {
            unsigned int j;
            for (j=0; j<bytes_read; j++) {
                CHECK(scratch[j] == (unsigned char)(offset+j));
            }
        }
        offset += bytes_read;
    }

    ATX_RELEASE_OBJECT(stream);
    ATX_RELEASE_OBJECT(source);
}

#if defined(_DEBUG)
#include <crtdbg.h>
#endif

/*----------------------------------------------------------------------
|    main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    int i, j;

#if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF    |
                   _CRTDBG_CHECK_ALWAYS_DF |
                   _CRTDBG_LEAK_CHECK_DF);
#endif

    ATX_System_SetRandomSeed(0);
    for (i=2; i<1000; i++) {
        for (j=1; j<3000; j++) {
            Test(i,j);
        }
    }
    for (i=2; i<100000; i+=77) {
        for (j=1; j<3000000; j+=777) {
            Test(i,j);
        }
    }
    
    return 0;
}