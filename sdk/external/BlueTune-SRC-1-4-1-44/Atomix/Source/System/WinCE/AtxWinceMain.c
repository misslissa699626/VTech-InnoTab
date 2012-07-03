/*****************************************************************
|
|   Atomix - main adapter for WinCE
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
#include <windows.h>

/*----------------------------------------------------------------------
|   imports
+---------------------------------------------------------------------*/
extern int main(int argc, char** argv);

/*----------------------------------------------------------------------
|   _tmain
+---------------------------------------------------------------------*/
int
_tmain(int argc, wchar_t** argv, wchar_t** envp)
{
    char** argv_utf8 = (char**)malloc((1+argc)*sizeof(char*));
    int i;
    int result;

    /* allocate and convert args */
    for (i=0; i<argc; i++) {
        unsigned int arg_length = wcslen(argv[i]);
        argv_utf8[i] = (char*)malloc(4*arg_length+1);
        WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, argv_utf8[i], 4*arg_length+1, 0, 0);
    }

    /* terminate the array with a null pointer */
    argv_utf8[argc] = NULL;

    /* call the real main */
    result = main(argc, argv_utf8);

    /* cleanup */
    for (i=0; i<argc; i++) {
        free(argv_utf8[i]);
    }
    free(argv_utf8);

    return result;
}
