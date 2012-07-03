/*****************************************************************
|
|   Atomix - Win32 Threads
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
#include "AtxThreads.h"
#include "AtxLogging.h"
#include "AtxUtils.h"

/*----------------------------------------------------------------------
|   logger
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("atomix.win32.threads")

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
struct ATX_Mutex {
    CRITICAL_SECTION mutex;
};

/*----------------------------------------------------------------------
|   ATX_Mutex_Create
+---------------------------------------------------------------------*/
ATX_Result
ATX_Mutex_Create(ATX_Mutex** mutex)
{
    if (mutex == NULL) {
        ATX_CHECK_WARNING(ATX_ERROR_INVALID_PARAMETERS);
    }
    *mutex = ATX_AllocateZeroMemory(sizeof(ATX_Mutex));
    if (*mutex == NULL) {
        ATX_CHECK_SEVERE(ATX_ERROR_OUT_OF_MEMORY);
    }
    InitializeCriticalSection(&(*mutex)->mutex);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_Mutex_LockAutoCreate
+---------------------------------------------------------------------*/
ATX_Result
ATX_Mutex_LockAutoCreate(ATX_Mutex** mutex)
{
    ATX_Mutex* tmp_mutex;
    void*      old_mutex;
    if (mutex == NULL) {
        ATX_CHECK_WARNING(ATX_ERROR_INVALID_PARAMETERS);
    }

    ATX_CHECK_WARNING(ATX_Mutex_Create(&tmp_mutex));
    old_mutex = InterlockedCompareExchangePointer((void**)mutex, 
                                                  (void*)tmp_mutex, 
                                                  NULL);
    if (old_mutex != NULL) {
        /* *mutex was not NULL: no exchange has been performed */
        ATX_Mutex_Destroy(tmp_mutex);
    }

    /* lock */
    EnterCriticalSection(&(*mutex)->mutex);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_Mutex_Lock
+---------------------------------------------------------------------*/
ATX_Result
ATX_Mutex_Lock(ATX_Mutex* self)
{
    if (self == NULL) {
        ATX_CHECK_WARNING(ATX_ERROR_INVALID_PARAMETERS);
    }
    EnterCriticalSection(&self->mutex);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_Mutex_Unlock
+---------------------------------------------------------------------*/
ATX_Result
ATX_Mutex_Unlock(ATX_Mutex* self)
{
    if (self == NULL) {
        ATX_CHECK_WARNING(ATX_ERROR_INVALID_PARAMETERS);
    }
    LeaveCriticalSection(&self->mutex);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_Mutex_Destroy
+---------------------------------------------------------------------*/
ATX_Result
ATX_Mutex_Destroy(ATX_Mutex* self)
{
    if (self == NULL) return ATX_SUCCESS; 
    DeleteCriticalSection(&self->mutex);
    ATX_FreeMemory(self);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_GetCurrentThreadId
+---------------------------------------------------------------------*/
ATX_ThreadId
ATX_GetCurrentThreadId(void)
{
    return GetCurrentThreadId();
}
