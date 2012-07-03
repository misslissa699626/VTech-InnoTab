/*****************************************************************
|
|   Stream Packetizer Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_STREAM_PACKETIZER_H_
#define _BLT_STREAM_PACKETIZER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_general_modules
 * @defgroup stream_packetizer_module Stream Packetizer Module 
 * Plugin module creates media nodes that split byte streams into
 * media packets. This module is typically automatically invoked by 
 * the stream manager to connect a media node that produces a byte stream
 * to a media node that expects media packets.
 * @{ 
 */

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "BltTypes.h"
#include "BltModule.h"

/*----------------------------------------------------------------------
|   module
+---------------------------------------------------------------------*/
BLT_Result BLT_StreamPacketizerModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_STREAM_PACKETIZER_H_ */
