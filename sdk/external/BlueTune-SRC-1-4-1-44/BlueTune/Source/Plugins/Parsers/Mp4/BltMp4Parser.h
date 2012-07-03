/*****************************************************************
|
|   Mp4Parser Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_MP4_PARSER_H_
#define _BLT_MP4_PARSER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_parser_modules
 * @defgroup mp4_parser_module MP4 Parser Module 
 * Plugin module that creates media nodes that parse the MP4 file format. 
 * These media nodes can handle all the compliant ISO MP4 (14496-12) 
 * formatted streams. 
 * These media nodes expect a byte stream input with ISO MP4 formatted data.
 * They produce media packets that contain the audio access units of the 
 * first audio track of its input.
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
#if defined(__cplusplus)
extern "C" {
#endif

BLT_Result BLT_Mp4ParserModule_GetModuleObject(BLT_Module** module);

#if defined(__cplusplus)
}
#endif

/** @} */

#endif /* _BLT_MP4_PARSER_H_ */
