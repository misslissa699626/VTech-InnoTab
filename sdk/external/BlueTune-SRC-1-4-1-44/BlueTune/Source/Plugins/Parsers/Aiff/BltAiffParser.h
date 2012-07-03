/*****************************************************************
|
|   AIFF Parser Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_AIFF_PARSER_H_
#define _BLT_AIFF_PARSER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_parser_modules
 * @defgroup aiff_parser_module AIFF Parser Module 
 * Plugins module that creates media nodes that can parse AIFF files.
 * These media nodes expect a byte stream as their input.
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
BLT_Result BLT_AiffParserModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_AIFF_PARSER_H_ */
