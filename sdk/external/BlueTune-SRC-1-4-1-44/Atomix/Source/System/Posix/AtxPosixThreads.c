/*****************************************************************
|
|   Atomix - Posix Threads
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
#include <pthread.h>
#include "AtxThreads.h"
#include "AtxLogging.h"
#include "AtxUtils.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
struct ATX_Mutex {
    pthread_mutex_t mutex;
};

/*----------------------------------------------------------------------
|   logger
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("atomix.posix.threads")

/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/
static pthread_mutex_t atx_global_lock = PTHREAD_MUTEX_INITIALIZER;

/*----------------------------------------------------------------------
|   ATX_GetCurrentThreadId
+---------------------------------------------------------------------*/
ATX_ThreadId
ATX_GetCurrentThreadId(void)
{
    pthread_t pid = pthread_self();
    return (ATX_ThreadId)((void*)pid);
}

/*----------------------------------------------------------------------
|   ATX_Mutex_Create
+---------------------------------------------------------------------*/
ATX_Result
ATX_Mutex_Create(ATX_Mutex** mutex)
{
    int pres;
    if (mutex == NULL) {
        ATX_CHECK_WARNING(ATX_ERROR_INVALID_PARAMETERS);
    }
    *mutex = ATX_AllocateZeroMemory(sizeof(ATX_Mutex));
    pres = pthread_mutex_init(&(*mutex)->mutex, NULL);
    if (pres != 0) {
        ATX_LOG_SEVERE_1("pthread mutex init failed with error %d", pres);
        return ATX_FAILURE;
    }
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_Mutex_LockAutoCreate
+---------------------------------------------------------------------*/
ATX_Result 
ATX_Mutex_LockAutoCreate(ATX_Mutex** mutex)
{
    ATX_Result result = ATX_SUCCESS;
    int pres;

    if (mutex == NULL) {
        ATX_CHECK_WARNING(ATX_ERROR_INVALID_PARAMETERS);
    }
    
    if (*mutex == NULL) { /* slight optimization */
        /* lock global */
        pthread_mutex_lock(&atx_global_lock);

        /* creation */
        if (*mutex == NULL) {
            result = ATX_Mutex_Create(mutex);
        }
    
        /* unlock global */
        pthread_mutex_unlock(&atx_global_lock);
    }
    ATX_CHECK_WARNING(result);

    /* go on and lock */
    pres = pthread_mutex_lock(&(*mutex)->mutex);
    if (pres != 0) {
        ATX_LOG_SEVERE_1("pthread mutex lock failed with error %d", pres);
        result = ATX_FAILURE;
    }
    return result;
}

/*----------------------------------------------------------------------
|   ATX_Mutex_Lock
+---------------------------------------------------------------------*/
ATX_Result
ATX_Mutex_Lock(ATX_Mutex* mutex)
{
    int pres;
    if (mutex == NULL) {
       ATX_CHECK_WARNING(ATX_ERROR_INVALID_PARAMETERS);
    }

    pres = pthread_mutex_lock(&mutex->mutex);
    if (pres != 0) {
        ATX_LOG_SEVERE_1("pthread mutex lock failed with error %d", pres);
        return ATX_FAILURE;
    }
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_Mutex_Unlock
+---------------------------------------------------------------------*/
ATX_Result
ATX_Mutex_Unlock(ATX_Mutex* mutex)
{
    int pres;
    if (mutex == NULL) {
       ATX_CHECK_WARNING(ATX_ERROR_INVALID_PARAMETERS);
    }

    pres = pthread_mutex_unlock(&mutex->mutex);
    if (pres != 0) {
        ATX_LOG_SEVERE_1("pthread mutex unlock failed with error %d", pres);
        return ATX_FAILURE;
    }
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_Mutex_Destroy
+---------------------------------------------------------------------*/
ATX_Result
ATX_Mutex_Destroy(ATX_Mutex* mutex)
{
    if (mutex == NULL) return ATX_SUCCESS;
    pthread_mutex_destroy(&mutex->mutex);
    ATX_FreeMemory(mutex);
    return ATX_SUCCESS;
}
        
