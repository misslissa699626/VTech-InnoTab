/*****************************************************************
|
|   DirectX 9 Video Output Module
|
|   (c) 2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_DX9_VIDEO_OUTPUT_H_
#define _BLT_DX9_VIDEO_OUTPUT_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_output_modules
 * @defgroup dx9_video_output_module DirectX 9 Video Output Module 
 * Plugin module that creates media nodes that can display video pixels
 * to screen using the DirectX's Direct3D interface.
 * These media nodes expect media packets with raw video.
 * This module responds to probe with the name:
 * 'dx9:<n>' where <n> is the window handle (HWND) in which the video
 * will be rendered, or "0" to create a new window.
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

BLT_Result BLT_Dx9VideoOutputModule_GetModuleObject(BLT_Module** module);

#if defined(__cplusplus)
}
#endif

/** @} */

#endif /* _BLT_DX9_VIDEO_OUTPUT_H_ */
