/*****************************************************************
|
|   Packet Streamer Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_PACKET_STREAMER_H_
#define _BLT_PACKET_STREAMER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_general_modules
 * @defgroup packet_streamer_module Packet Streamer Module 
 * Plugin module creates media nodes that convert media packets into
 * a byte stream. This module is typically automatically invoked by 
 * the stream manager to connect a media node that produces packets to
 * a media node that expects a byte stream.
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
BLT_Result BLT_PacketStreamerModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_PACKET_STREAMER_H_ */
