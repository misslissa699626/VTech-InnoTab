/*****************************************************************
|
|   BlueTune - TCP Network Stream
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

#ifndef _BLT_TCP_NETWORK_STREAM_H_
#define _BLT_TCP_NETWORK_STREAM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "BltTypes.h"
#include "BltModule.h"

/*----------------------------------------------------------------------
|   module
+---------------------------------------------------------------------*/
BLT_Result 
BLT_TcpNetworkStream_Create(const char* name, ATX_InputStream** stream);

#endif /* _BLT_TCP_NETWORK_STREAM_H_ */
