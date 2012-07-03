/*****************************************************************
|
|   WaveFormatter Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_WAVE_FORMATTER_H_
#define _BLT_WAVE_FORMATTER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_formatter_modules
 * @defgroup wave_formatter_module Wave Formatter Module 
 * Plugin module that creates media nodes that format WAV (RIFF variant)
 * encoded streams with PCM audio.
 * These media nodes expect a byte stream with PCM audio and produce
 * a byte stream.
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
BLT_Result BLT_WaveFormatterModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_WAVE_FORMATTER_H_ */
