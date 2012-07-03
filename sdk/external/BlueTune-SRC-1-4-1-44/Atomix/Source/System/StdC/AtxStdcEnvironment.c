/*****************************************************************
|
|      Atomix - Environment variables: StdC Implementation
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
#include <stdlib.h>

#include "AtxConfig.h"
#include "AtxUtils.h"
#include "AtxResults.h"

/*----------------------------------------------------------------------
|   ATX_GetEnvironment
+---------------------------------------------------------------------*/
ATX_Result 
ATX_GetEnvironment(const char* name, ATX_String* value)
{
    char* env;

    /* default value */
    ATX_String_SetLength(value, 0);

#if defined(ATX_CONFIG_HAVE_GETENV)
    env = getenv(name);
    if (env) {
        ATX_String_Assign(value, env);
        return ATX_SUCCESS;
    } else {
        return ATX_ERROR_NO_SUCH_ITEM;
    }
#elif defined(ATX_CONFIG_HAVE_DUPENV_S)
    if (dupenv_s(&env, NULL, name) != 0) {
        return ATX_FAILURE;
    } else if (env != NULL) {
        ATX_String_Assign(value, env);
        free(env);
        return ATX_SUCCESS;
    } else {
        return ATX_ERROR_NO_SUCH_ITEM;
    }
#else
#error "no getenv or getenv_s available on this platform"
#endif
}
