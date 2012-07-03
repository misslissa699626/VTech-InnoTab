/*****************************************************************
|
|   BlueTune - Time Library
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltTime.h"

/*----------------------------------------------------------------------
|   BLT_TimeStamp_Add
+---------------------------------------------------------------------*/
BLT_TimeStamp 
BLT_TimeStamp_Add(BLT_TimeStamp t0, BLT_TimeStamp t1)
{
    BLT_TimeStamp result;
    result.seconds     = t0.seconds     + t1.seconds;
    result.nanoseconds = t0.nanoseconds + t1.nanoseconds;
    if (result.nanoseconds > 1000000000) {
        result.seconds     += 1;
        result.nanoseconds -= 1000000000;
    }
    
    return result;
}

/*----------------------------------------------------------------------
|   BLT_TimeStamp_Sub
+---------------------------------------------------------------------*/
BLT_TimeStamp
BLT_TimeStamp_Sub(BLT_TimeStamp t0, BLT_TimeStamp t1)
{
    BLT_TimeStamp result;
    result.seconds     = t0.seconds     - t1.seconds;
    result.nanoseconds = t0.nanoseconds - t1.nanoseconds;
    if (result.nanoseconds < 0) {
        result.seconds     -= 1;
        result.nanoseconds += 1000000000;
    }
    
    return result;
}

/*----------------------------------------------------------------------
|   BLT_TimeStamp_ToSeconds
+---------------------------------------------------------------------*/
double
BLT_TimeStamp_ToSeconds(BLT_TimeStamp t)
{
    return (double)t.seconds+(double)t.nanoseconds/1000000000.0f;
}

/*----------------------------------------------------------------------
|   BLT_TimeStamp_ToNanos
+---------------------------------------------------------------------*/
ATX_UInt64
BLT_TimeStamp_ToNanos(BLT_TimeStamp t)
{
    return 1000000000*(ATX_UInt64)t.seconds+t.nanoseconds;
}

/*----------------------------------------------------------------------
|   BLT_TimeStamp_ToMicros
+---------------------------------------------------------------------*/
ATX_UInt64
BLT_TimeStamp_ToMicros(BLT_TimeStamp t)
{
    return 1000000*(ATX_UInt64)t.seconds+(t.nanoseconds/1000);
}

/*----------------------------------------------------------------------
|   BLT_TimeStamp_ToMillis
+---------------------------------------------------------------------*/
ATX_UInt64
BLT_TimeStamp_ToMillis(BLT_TimeStamp t)
{
    return 1000*(ATX_UInt64)t.seconds+(t.nanoseconds/1000000);
}

/*----------------------------------------------------------------------
|   BLT_TimeStamp_FromSeconds
+---------------------------------------------------------------------*/
BLT_TimeStamp 
BLT_TimeStamp_FromSeconds(double seconds)
{
    return BLT_TimeStamp_FromNanos((ATX_UInt64)(1000000000.0*seconds));
}

/*----------------------------------------------------------------------
|   BLT_TimeStamp_FromNanos
+---------------------------------------------------------------------*/
BLT_TimeStamp 
BLT_TimeStamp_FromNanos(ATX_UInt64 nanos)
{
    BLT_TimeStamp result;
    result.seconds     = (ATX_Int32)(nanos/1000000000);
    result.nanoseconds = (ATX_Int32)(nanos%1000000000);
    
    return result;
}

/*----------------------------------------------------------------------
|   BLT_TimeStamp_FromMicros
+---------------------------------------------------------------------*/
BLT_TimeStamp 
BLT_TimeStamp_FromMicros(ATX_UInt64 micros)
{
    BLT_TimeStamp result;
    result.seconds     = (ATX_Int32)(micros/1000000);
    result.nanoseconds = (ATX_Int32)(1000*(micros%1000000));

    return result;
}

/*----------------------------------------------------------------------
|   BLT_TimeStamp_FromMillis
+---------------------------------------------------------------------*/
BLT_TimeStamp 
BLT_TimeStamp_FromMillis(ATX_UInt64 millis)
{
    BLT_TimeStamp result;
    result.seconds     = (ATX_Int32)(millis/1000);
    result.nanoseconds = (ATX_Int32)(1000000*(millis%1000));
    
    return result;
}

/*----------------------------------------------------------------------
|   BLT_TimeStamp_FromSamples
+---------------------------------------------------------------------*/
BLT_TimeStamp 
BLT_TimeStamp_FromSamples(ATX_Int64 sample_count,
                          ATX_Int32 sample_rate)
{
    return BLT_TimeStamp_FromMicros((sample_count*1000000)/sample_rate);
}
