/*****************************************************************
|
|   M4aParser Module
|
|   (c) 2002-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_M4A_PARSER_H_
#define _BLT_M4A_PARSER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_parser_modules
 * @defgroup m4a_parser_module M4A Parser Module 
 * Plugin module that creates media nodes that parse AAC M4A 
 * encoded streams.
 * These media nodes expect a byte stream with M4A encoded data and produce
 * packets with AAC frames. This module registers the mime types:
 * audio/aac and audio/aacp
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
BLT_Result BLT_M4aParserModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_M4A_PARSER_H_ */
