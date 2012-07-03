/*****************************************************************
|
|   BlueTune - Network Stream
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

#ifndef _BLT_NETWORK_STREAM_H_
#define _BLT_NETWORK_STREAM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltTypes.h"

/*----------------------------------------------------------------------
|   functions
+---------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif

BLT_Result 
BLT_NetworkStream_Create(BLT_Size          size,
                         ATX_InputStream*  source, 
                         ATX_InputStream** stream);

#if defined(__cplusplus)
}
#endif

#endif /* _BLT_NETWORK_STREAM_H_ */
