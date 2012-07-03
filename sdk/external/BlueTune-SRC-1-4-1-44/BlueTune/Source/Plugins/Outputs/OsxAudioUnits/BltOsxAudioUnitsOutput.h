/*****************************************************************
|
|   OSX Audio Units Output Module
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_OSX_AUDIO_UNITS_OUTPUT_H_
#define _BLT_OSX_AUDIO_UNITS_OUTPUT_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_output_modules
 * @defgroup macosx_output_module MacOSX Output Module 
 * Plugin module that creates media nodes that can send PCM audio data
 * to a sound card on MacOSX.
 * These media nodes expect media pakcets with PCM audio.
 * This module responds to probe with the name:
 * 'osxau:<n>' or 'osxau:#<name>'
 * <n> is an integer, to selects the device by index, 
 * where 0 means the default output, 1 the first output, etc...
 * <name> is the name of an output device (ex: 'Built-in Output')
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
BLT_Result BLT_OsxAudioUnitsOutputModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_OSX_AUDIO_UNITS_OUTPUT_H_ */
