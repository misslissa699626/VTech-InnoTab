/*****************************************************************
|
|   AdtsParser Module
|
|   (c) 2002-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_ADTS_PARSER_H_
#define _BLT_ADTS_PARSER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_parser_modules
 * @defgroup adts_parser_module ADTS Parser Module 
 * Plugin module that creates media nodes that parse AAC ADTS 
 * encoded streams.
 * These media nodes expect a byte stream with ADTS encoded data and produce
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
BLT_Result BLT_AdtsParserModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_ADTS_PARSER_H_ */
