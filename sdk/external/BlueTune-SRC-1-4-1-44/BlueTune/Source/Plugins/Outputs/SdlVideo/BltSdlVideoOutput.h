/*****************************************************************
|
|   SdlVideoOutput Module
|
|   (c) 2008-2009 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_SDL_VIDEO_OUTPUT_H_
#define _BLT_SDL_VIDEO_OUTPUT_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_output_modules
 * @defgroup sdl_output_module SDL Output Module 
 * Plugin module that creates media nodes that can display video pixels
 * to onscreen using the SDL library.
 * These media nodes expect media pakcets with raw video.
 * This module responds to probe with the name:
 * 'sdl:<n>'
 * (In this version, <n> is ignored, and the default screen will
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
BLT_Result BLT_SdlVideoOutputModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_SDL_VIDEO_OUTPUT_H_ */
