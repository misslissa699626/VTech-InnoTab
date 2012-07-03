/*****************************************************************
|
|   WaveParser Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_WAVE_PARSER_H_
#define _BLT_WAVE_PARSER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_parser_modules
 * @defgroup wave_parser_module Wave Parser Module 
 * Plugin module that creates media nodes that parse WAV (RIFF variant)
 * encoded streams with PCM audio.
 * These media nodes expect a byte stream with WAV encoded data and produce
 * a byte stream with PCM audio data.
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
BLT_Result BLT_WaveParserModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_WAVE_PARSER_H_ */
