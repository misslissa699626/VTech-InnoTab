/*****************************************************************
|
|   Atomix - Time
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
/** @file
 * Atomix Time Interface Header file
 */

#ifndef _ATX_TIME_H_
#define _ATX_TIME_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "AtxConfig.h"
#include "AtxTypes.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct {
    ATX_Int32 seconds;
    ATX_Int32 nanoseconds;
} ATX_TimeStamp;

typedef ATX_TimeStamp ATX_Time;
typedef ATX_TimeStamp ATX_TimeInterval;

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#define ATX_TimeStamp_Set(t, s, n) \
    do {(t).seconds = (s); (t).nanoseconds = (n);} while(0)

#define ATX_TimeStamp_IsLater(t0,t1)           \
(                                              \
    ((t0).seconds > (t1).seconds) ||    \
    (                                          \
        (t0).seconds == (t1).seconds &&        \
        (t0).nanoseconds > (t1).nanoseconds    \
    )                                          \
)

#define ATX_TimeStamp_IsLaterOrEqual(t0,t1)   \
(                                             \
    ((t0).seconds > (t1).seconds) ||   \
    (                                         \
        (t0).seconds == (t1).seconds &&       \
        (t0).nanoseconds >= (t1).nanoseconds  \
    )                                         \
)

#define ATX_TimeStamp_Add(t0,t1,t2) do {                        \
    (t0).seconds = (t1).seconds + (t2).seconds;                 \
    (t0).nanoseconds = (t1).nanoseconds + (t2).nanoseconds;     \
    if ((t0).nanoseconds > 1000000000) {                        \
        (t0).seconds++;                                         \
        (t0).nanoseconds -= 1000000000;                         \
    }                                                           \
} while (0)

#define ATX_TimeStamp_Sub(t0,t1,t2) do {                        \
    (t0).seconds = (t1).seconds - (t2).seconds;                 \
    (t0).nanoseconds = (t1).nanoseconds - (t2).nanoseconds;     \
    if ((t0).nanoseconds < 0) {                                 \
        (t0).seconds--;                                         \
        (t0).nanoseconds += 1000000000;                         \
    }                                                           \
} while (0)

#define ATX_TimeStamp_ToInt64(t, i) do {                        \
    ATX_Int64_Set_Int32(i, t.seconds);                          \
    ATX_Int64_Mul_Int32(i, 1000000000);                         \
    ATX_Int64_Add_Int32(i, t.nanoseconds);                      \
} while (0)

#endif /* _ATX_TIME_H_ */
