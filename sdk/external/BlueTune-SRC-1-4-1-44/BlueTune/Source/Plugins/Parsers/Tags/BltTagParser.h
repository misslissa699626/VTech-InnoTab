/*****************************************************************
|
|   Tag Parser Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_TAG_PARSER_H_
#define _BLT_TAG_PARSER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_parser_modules
 * @defgroup tag_parser_module Tag Parser Module 
 * Plugin module creates media nodes that parse MP3 meta-data (tags)
 * found in MP3 audio stream.
 * This module supports ID3 tags, Xing and Fraunhofer headers, LAME
 * encoder tags, and knows how to skip ID3v2 headers and footers.
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
BLT_Result BLT_TagParserModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_TAG_PARSER_H_ */
