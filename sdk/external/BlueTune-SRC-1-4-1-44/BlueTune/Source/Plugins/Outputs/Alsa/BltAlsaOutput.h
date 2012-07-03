/*****************************************************************
|
|      File: BltAlsaOutput.h
|
|      ALSA Output Module
|
|      (c) 2002-2004 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_ALSA_OUTPUT_H_
#define _BLT_ALSA_OUTPUT_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_output_modules
 * @defgroup alsa_output_module ALSA Output Module 
 * Plugin module that creates media nodes that can send PCM audio data
 * to a sound card using an ALSA driver.
 * These media nodes expect media pakcets with PCM audio.
 * This module responds to probe with the name:
 * 'alsa:<name>'
 * where <name> is the name of an ALSA output.
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
extern BLT_Result BLT_AlsaOutputModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_ALSA_OUTPUT_H_ */
