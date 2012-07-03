/*****************************************************************
|
|   Atomix - System, Win32 Implementation
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
#if !defined(_WIN32_WCE)
#include <sys/timeb.h>
#endif

#include "AtxConfig.h"
#include "AtxTypes.h"
#include "AtxSystem.h"
#include "AtxResults.h"
#include "AtxDebug.h"

/*----------------------------------------------------------------------
|   singleton
+---------------------------------------------------------------------*/
static ATX_Boolean ATX_System_RandomGeneratorSeeded = ATX_FALSE;

#if defined(_WIN32_WCE)
/*----------------------------------------------------------------------
|   ATX_System_GetCurrentTimeStamp
+---------------------------------------------------------------------*/
ATX_Result
ATX_System_GetCurrentTimeStamp(ATX_TimeStamp* now)
{
    SYSTEMTIME stime;
    FILETIME   ftime;
    __int64    time64;
    GetSystemTime(&stime);
    SystemTimeToFileTime(&stime, &ftime);

    /* convert to 64-bits 100-nanoseconds value */
    time64 = (((unsigned __int64)ftime.dwHighDateTime)<<32) | ((unsigned __int64)ftime.dwLowDateTime);
    time64 -= 116444736000000000; /* convert from the Windows epoch (Jan. 1, 1601) to the 
                                   * Unix epoch (Jan. 1, 1970) */
    
    now->seconds = (ATX_Int32)(time64/10000000);
    now->nanoseconds = 100*(ATX_Int32)(time64-((unsigned __int64)now->seconds*10000000));

    return ATX_SUCCESS;
}
#else
/*----------------------------------------------------------------------
|   ATX_System_GetCurrentTimeStamp
+---------------------------------------------------------------------*/
ATX_Result
ATX_System_GetCurrentTimeStamp(ATX_TimeStamp* now)
{
    struct _timeb time_stamp;

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
    _ftime_s(&time_stamp);
#else
    _ftime(&time_stamp);
#endif
    now->seconds     = (long)time_stamp.time;
    now->nanoseconds = (long)time_stamp.millitm*1000000;

    return ATX_SUCCESS;
}
#endif

/*----------------------------------------------------------------------
|   ATX_System_Sleep
+---------------------------------------------------------------------*/
ATX_Result
ATX_System_Sleep(const ATX_TimeInterval* duration)
{
    DWORD milliseconds = 1000*duration->seconds + duration->nanoseconds/1000000;
    Sleep(milliseconds);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_System_SleepUntil
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

