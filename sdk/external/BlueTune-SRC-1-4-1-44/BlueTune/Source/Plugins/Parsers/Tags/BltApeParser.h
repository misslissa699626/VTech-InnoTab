/*****************************************************************
|
|   APE Tag Parser Library
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_APE_PARSER_H_
#define _BLT_APE_PARSER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltTypes.h"
#include "BltStream.h"
#include "BltEventListener.h"

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
BLT_Result BLT_ApeParser_ParseStream(ATX_InputStream* stream,
                                     BLT_Offset       stream_start,
                                     BLT_Size         stream_size,
                                     BLT_Size*        trailer_size,
                                     ATX_Properties*  properties);

#endif /* _BLT_APE_PARSER_H_ */
