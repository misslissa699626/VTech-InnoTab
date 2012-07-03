/*****************************************************************
|
|   BlueTune - Time Definitions
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Time API
 */

#ifndef _BLT_TIME_H_
#define _BLT_TIME_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltDefs.h"
#include "BltTypes.h"
#include "BltErrors.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct {
    BLT_UInt8 h;
    BLT_UInt8 m;
    BLT_UInt8 s;
    BLT_UInt8 f;
} BLT_TimeCode;

typedef struct {
    BLT_Int32 seconds;
    BLT_Int32 nanoseconds;
} BLT_TimeStamp;

typedef BLT_TimeStamp BLT_Time;

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#define BLT_TimeStamp_Set(t, s, n) \
    do {(t).seconds = (s); (t).nanoseconds = (n);} while(0)

#define BLT_TimeStamp_IsLater(t0,t1)           \
(                                              \
    ((t0).seconds > (t1).seconds) ||    \
    (                                          \
        (t0).seconds == (t1).seconds &&        \
        (t0).nanoseconds > (t1).nanoseconds    \
    )                                          \
)

#define BLT_TimeStamp_IsLaterOrEqual(t0,t1)       \
(                                                 \
    (                                             \
        (t0).seconds == (t1).seconds &&           \
        (t0).nanoseconds == (t1).nanoseconds      \
    ) ||                                   \
    (                                             \
        ((t0).seconds > (t1).seconds) ||   \
        (                                         \
            (t0).seconds == (t1).seconds &&       \
            (t0).nanoseconds > (t1).nanoseconds   \
        )                                         \
    )                                             \
)

/*----------------------------------------------------------------------
|   functions
+---------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif

BLT_TimeStamp BLT_TimeStamp_Add(BLT_TimeStamp t0, BLT_TimeStamp t1);
BLT_TimeStamp BLT_TimeStamp_Sub(BLT_TimeStamp t1, BLT_TimeStamp t2);
BLT_TimeStamp BLT_TimeStamp_FromSeconds(double seconds);
BLT_TimeStamp BLT_TimeStamp_FromNanos(ATX_UInt64 nanos);
BLT_TimeStamp BLT_TimeStamp_FromMicros(ATX_UInt64 micros);
BLT_TimeStamp BLT_TimeStamp_FromMillis(ATX_UInt64 millis);
double        BLT_TimeStamp_ToSeconds(BLT_TimeStamp ts);
ATX_UInt64    BLT_TimeStamp_ToNanos(BLT_TimeStamp ts);
ATX_UInt64    BLT_TimeStamp_ToMicros(BLT_TimeStamp ts);
ATX_UInt64    BLT_TimeStamp_ToMillis(BLT_TimeStamp ts);
BLT_TimeStamp BLT_TimeStamp_FromSamples(ATX_Int64 sample_count,
                                        ATX_Int32 sample_rate);

#if defined(__cplusplus)
}
#endif

#endif /* _BLT_TIME_H_ */
