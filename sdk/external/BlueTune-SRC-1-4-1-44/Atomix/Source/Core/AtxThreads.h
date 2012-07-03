/*****************************************************************
|
|   Atomix - Threads
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

#ifndef _ATX_THREADS_H_
#define _ATX_THREADS_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "AtxTypes.h"
#include "AtxResults.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct ATX_Mutex ATX_Mutex;
typedef unsigned long    ATX_ThreadId;

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a mutex object
 */
ATX_Result
ATX_Mutex_Create(ATX_Mutex** mutex);

/*
 * Atomix mutext locking/creation 
 *
 * This is useful for creating/locking mutexes that are used as
 * singletons for example.
 * if *mutex is NULL, the mutex will be created in a thread safe manner
 * (it is ok to have several threads try to create the same mutex at 
 * the same time), and will then locked. Otherwise, it will just be locked.
 */
ATX_Result
ATX_Mutex_LockAutoCreate(ATX_Mutex** mutex);

ATX_Result
ATX_Mutex_Lock(ATX_Mutex* mutex);

ATX_Result
ATX_Mutex_Unlock(ATX_Mutex* mutex);

ATX_Result
ATX_Mutex_Destroy(ATX_Mutex* mutex);

ATX_ThreadId
ATX_GetCurrentThreadId(void);

#ifdef __cplusplus
}
#endif

#endif /* _ATX_THREADS_H_ */

