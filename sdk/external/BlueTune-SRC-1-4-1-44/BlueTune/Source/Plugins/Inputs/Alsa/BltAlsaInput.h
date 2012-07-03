/*****************************************************************
|
|      File: BltAlsaInput.h
|
|      ALSA Input Module
|
|      (c) 2002-2004 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_ALSA_INPUT_H_
#define _BLT_ALSA_INPUT_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_input_modules
 * @defgroup alsa_input_module ALSA Input Module 
 * Plugin module that creates media nodes that can read PCM audio data
 * from a sound card using an ALSA driver.
 * These media nodes produce media pakcets with PCM audio.
 * This module responds to probe with the name:
 * 'alsa:<name>'
 * where <name> is the name of an ALSA input.
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
extern BLT_Result BLT_AlsaInputModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_ALSA_INPUT_H_ */
