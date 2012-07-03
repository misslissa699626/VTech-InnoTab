/*****************************************************************
|
|   Atomix - System, PS3 Implementation
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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/timer.h>
#include <sys/sys_time.h>
#include <unistd.h>

#include "AtxConfig.h"
#include "AtxTypes.h"
#include "AtxSystem.h"
#include "AtxResults.h"
#include "AtxDebug.h"

/*----------------------------------------------------------------------
|   singleton
+---------------------------------------------------------------------*/
static ATX_Boolean ATX_System_RandomGeneratorSeeded = ATX_FALSE;

/*----------------------------------------------------------------------
|   ATX_System_GetCurrentTimeStamp
+---------------------------------------------------------------------*/
ATX_Result
ATX_System_GetCurrentTimeStamp(ATX_TimeStamp* now)
{
    sys_time_sec_t sec;
    sys_time_nsec_t nsec;

    int result = sys_time_get_current_time(&sec, &nsec);
    if (result != CELL_OK){
        now->seconds     = 0;
        now->nanoseconds = 0;   
        return ATX_FAILURE;
    }

    /* convert format */
    now->seconds     = sec;
    now->nanoseconds = nsec;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_System_Sleep
+---------------------------------------------------------------------*/
ATX_Result
ATX_System_Sleep(const ATX_TimeInterval* duration)
{
    unsigned long usecs = 1000000*duration->seconds + duration->nanoseconds/1000;
    sys_timer_usleep(usecs);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_System_SleepUntil
+---------------------------------------------------------------------*/
ATX_Result
ATX_System_SleepUntil(const ATX_TimeStamp* when)
{
    ATX_TimeStamp now;
    ATX_System_GetCurrentTimeStamp(&now);
    if (ATX_TimeStamp_IsLater(*when, now)) {
        ATX_TimeInterval duration;
        ATX_TimeStamp_Sub(duration, *when, now);
        return ATX_System_Sleep(&duration);
    } else {
        return ATX_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   ATX_System_SetRandomSeed
+---------------------------------------------------------------------*/
ATX_Result  
ATX_System_SetRandomSeed(unsigned int seed)
{
    srand(seed);
    ATX_System_RandomGeneratorSeeded = ATX_TRUE;
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_System::ATX_System
+---------------------------------------------------------------------*/
ATX_UInt32 
ATX_System_GetRandomInteger()
{
    if (!ATX_System_RandomGeneratorSeeded) {
        ATX_TimeStamp now;
        ATX_System_GetCurrentTimeStamp(&now);
        ATX_System_SetRandomSeed(now.nanoseconds);
    }

    return rand();
}


