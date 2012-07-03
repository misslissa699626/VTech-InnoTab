/*****************************************************************
|
|   Network Input Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_NETWORK_INPUT_H_
#define _BLT_NETWORK_INPUT_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_input_modules
 * @defgroup network_input_module Network Input Module 
 * Plugin module that creates media nodes that can read from network sources. 
 * These media nodes can handle http URLs.
 * These media nodes produce a byte stream, implement the necessary buffering
 * and set the mime type of the stream if it can be determined from the protocol.
 * If the media mime type cannot be determined from the protocol, the mime type
 * is guessed based on the input name.
 *
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
BLT_Result BLT_NetworkInputModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_NETWORK_INPUT_H_ */
