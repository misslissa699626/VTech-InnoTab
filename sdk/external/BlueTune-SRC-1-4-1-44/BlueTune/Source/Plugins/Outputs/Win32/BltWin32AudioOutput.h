/*****************************************************************
|
|   Win32 Audio Output Module
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_WIN32_AUDIO_OUTPUT_H_
#define _BLT_WIN32_AUDIO_OUTPUT_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_output_modules
 * @defgroup win32_output_module Win32 Output Module 
 * Plugin module that creates media nodes that can send PCM audio data
 * to a sound card on Windows.
 * These media nodes expect media packets with PCM audio.
 * This module responds to probe with the name:
 * 'wave:<n>'
 * (In this version, <n> is ignored, and the default sound card will
 * be used).
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
BLT_Result BLT_Win32AudioOutputModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_WIN32_AUDIO_OUTPUT_H_ */
