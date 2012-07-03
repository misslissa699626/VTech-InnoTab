/*****************************************************************
|
|      Cdda: BltCddaInput.h
|
|      Cdda Input Module
|
|      (c) 2002-2003 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_CDDA_INPUT_H_
#define _BLT_CDDA_INPUT_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_input_modules
 * @defgroup cdda_input_module CDDA Input Module 
 * [NOTE: this module is not available on all platforms]
 * Plugin module that creates media nodes that can read audio CD's.
 * These media nodes read from an audio CD and produce a byte stream
 * with PCM audio.
 * This module responds to probes with the name:
 * cdda:<n> 
 * where <n> is a track number.
 * @{ 
 */

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "BltTypes.h"
#include "BltModule.h"

/*----------------------------------------------------------------------
|       module
+---------------------------------------------------------------------*/
extern BLT_Result BLT_CddaInputModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_CDDA_INPUT_H_ */
