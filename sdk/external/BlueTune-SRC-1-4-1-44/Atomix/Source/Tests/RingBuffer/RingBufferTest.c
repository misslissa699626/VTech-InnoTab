/*****************************************************************
|
|      RingBuffer Test Program 1
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
#include <stdlib.h>
#include <stdio.h>

#if defined(_DEBUG) && defined(WIN32)
#include <crtdbg.h>
#endif

#define BUFFER_SIZE 17

/*----------------------------------------------------------------------
|       ReadChunk
+---------------------------------------------------------------------*/
static ATX_Result
ReadChunk(ATX_RingBuffer* buffer)
{
    static unsigned int total_read = 0;
    ATX_Result          result;
    unsigned int        i;
    unsigned char       bytes[BUFFER_SIZE];
    unsigned int        chunk = rand()%BUFFER_SIZE;
    unsigned int        can_read = ATX_RingBuffer_GetAvailable(buffer);
    if (chunk > can_read) chunk = can_read;
    if (chunk == 0) return ATX_SUCCESS;

    /* read a chunk */
    result = ATX_RingBuffer_Read(buffer, bytes, chunk);
    if (ATX_FAILED(result)) return result;

    /* check values */
    for (i=0; i<chunk; i++) {
        unsigned int index = total_read+i;
        unsigned char expected = index & 0xFF;
        if (bytes[i] != expected) {
            printf("unexpected byte at index %d (expected %d, got %d)\n", 
                   index, expected, bytes[i]);
            return ATX_FAILURE;
        }
    }
    total_read += chunk;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|       WriteChunk
+---------------------------------------------------------------------*/
static ATX_Result
WriteChunk(ATX_RingBuffer* buffer)
{
    static unsigned int total_written = 0;
    unsigned char       bytes[BUFFER_SIZE];
    ATX_Result          result;
    unsigned int        i;
    unsigned int        chunk = rand()%BUFFER_SIZE;
    unsigned int        can_write = ATX_RingBuffer_GetSpace(buffer);
    if (chunk > can_write) chunk = can_write;
    if (chunk == 0) return ATX_SUCCESS;

    /* generate buffer */
    for (i=0; i<chunk; i++) {
        unsigned int index = total_written+i;
        bytes[i] = index&0xFF;
    }

    /* write chunk */
    result = ATX_RingBuffer_Write(buffer, bytes, chunk);
    if (ATX_FAILED(result)) return result;
    total_written += chunk;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    ATX_Result      result;
    ATX_RingBuffer* buffer;
    int             i;

    /* setup debugging */
#if defined(_DEBUG) && defined(WIN32)
    int flags = _crtDbgFlag       | 
        _CRTDBG_ALLOC_MEM_DF      |
        _CRTDBG_DELAY_FREE_MEM_DF |
        _CRTDBG_CHECK_ALWAYS_DF;

    _CrtSetDbgFlag(flags);
    /*AllocConsole();
     freopen("CONOUT$", "w", stdout);*/
#endif 

    ATX_COMPILER_UNUSED(argc);
    ATX_COMPILER_UNUSED(argv);

    result = ATX_RingBuffer_Create(BUFFER_SIZE, &buffer);
    if (ATX_FAILED(result)) {
        fprintf(stderr, "ATX_RingBufferConstruct failed (%d)\n", result);
        return 1;
    }

    /* test a few basic functions */
    ATX_RingBuffer_Write(buffer, (ATX_ByteBuffer)"ab", 2);
    {
        ATX_ByteBuffer bb = ATX_RingBuffer_GetIn(buffer);
        *bb = 'c';
        ATX_RingBuffer_MoveIn(buffer, 1);
    }
    ATX_ASSERT(ATX_RingBuffer_GetSpace(buffer) == BUFFER_SIZE-3-1);
    ATX_ASSERT(ATX_RingBuffer_GetAvailable(buffer) == 3);
    ATX_ASSERT(ATX_RingBuffer_PeekByte(buffer, 2) == 'c');
    ATX_ASSERT(ATX_RingBuffer_ReadByte(buffer) == 'a');
    ATX_ASSERT(ATX_RingBuffer_ReadByte(buffer) == 'b');
    ATX_ASSERT(ATX_RingBuffer_ReadByte(buffer) == 'c');
    ATX_ASSERT(ATX_RingBuffer_GetAvailable(buffer) == 0);
    ATX_ASSERT(ATX_RingBuffer_GetSpace(buffer) == BUFFER_SIZE-1);
    ATX_ASSERT(ATX_RingBuffer_GetContiguousSpace(buffer) == BUFFER_SIZE-3);

    ATX_RingBuffer_Reset(buffer);
    for (i=0; i<100000000; i++) {
        if (ATX_FAILED(WriteChunk(buffer))) {
            printf("WriteChunk failed\n");
            return 1;
        }
        if (ATX_FAILED(ReadChunk(buffer))) {
            printf("ReadChunk failed\n");
            return 1;
        }
    }

    ATX_RingBuffer_Destroy(buffer);

    printf("RingBufferTest passed\n");

    return 0;
}
