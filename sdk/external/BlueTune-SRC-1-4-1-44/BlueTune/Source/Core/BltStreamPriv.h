/*****************************************************************
|
|   BlueTune - Stream Private API
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_STREAM_PRIV_H_
#define _BLT_STREAM_PRIV_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltDefs.h"
#include "BltTypes.h"
#include "BltErrors.h"
#include "BltCore.h"

/*----------------------------------------------------------------------
|   Stream_Create
+---------------------------------------------------------------------*/
BLT_Result Stream_Create(BLT_Core* core, BLT_Stream** stream);

#endif /* _BLT_STREAM_PRIV_H_ */
