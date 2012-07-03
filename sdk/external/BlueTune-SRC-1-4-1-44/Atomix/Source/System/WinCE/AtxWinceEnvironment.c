/*****************************************************************
|
|      Atomix - Environment variables: Windows CE Implementation
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
#include <windows.h>

#include "AtxConfig.h"
#include "AtxUtils.h"
#include "AtxResults.h"

/*----------------------------------------------------------------------
|   ATX_GetEnvironment
+---------------------------------------------------------------------*/
ATX_Result 
ATX_GetEnvironment(const char* name, ATX_String* value)
{
    HKEY       key = NULL; 
    DWORD      type;
    WCHAR*     name_w;
    DWORD      name_length;
    DWORD      value_length;
    ATX_Result result;

    /* default value */
    ATX_String_SetLength(value, 0);

    /* convert name to unicode */
    name_length = ATX_StringLength(name);
    name_w = (WCHAR*)ATX_AllocateMemory(sizeof(WCHAR)*(name_length+1));
    MultiByteToWideChar(CP_UTF8, 0, name, -1, name_w, name_length+1);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, 
                     _T("Software\\Axiomatic\\Atomix\\Environment"), 
                     0, KEY_ALL_ACCESS, &key) == ERROR_SUCCESS) { 
        if (RegQueryValueEx(key, name_w, 0, &type, (PBYTE)NULL, &value_length ) == ERROR_SUCCESS) { 
            /* convert to UTF-8 */

            WCHAR* value_w = ATX_AllocateMemory(sizeof(WCHAR)*(value_length+1));
            int    value_size = 4*value_length+1;
            ATX_String_Reserve(value, value_size);

            if (RegQueryValueEx(key, name_w, 0, &type, (PBYTE)value_w, &value_length ) == ERROR_SUCCESS) {
                value_size = WideCharToMultiByte(CP_UTF8, 0, value_w, value_length, ATX_String_UseChars(value), value_size, NULL, FALSE);
                ATX_String_SetLength(value, value_size);
            }

            ATX_FreeMemory(value_w);
            result = ATX_SUCCESS;
        }
    }

    ATX_FreeMemory(name_w);

    return result;
}
