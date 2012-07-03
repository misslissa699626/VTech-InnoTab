/*****************************************************************
|
|   Gain Control Filter Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_GAIN_CONTROL_FILTER_H_
#define _BLT_GAIN_CONTROL_FILTER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_filter_modules
 * @defgroup gain_control_filter_module Gain Control Filter Module 
 * Plugin module that create media nodes that perform gain control on
 * PCM audio data.
 * These media nodes expect media packets with PCM audio as input, 
 * and produce media packets with PCM audio as output.
 * They amplify or attenuate the PCM audio by a variable gain factor. 
 * These media nodes listen for ReplayGain values set by other nodes in 
 * the chain when they are found in certain media format or meta-data.
 *
 * @{ 
 */

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "BltTypes.h"
#include "BltModule.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_GAIN_CONTROL_FILTER_OPTION_DO_REPLAY_GAIN "Plugins.GainControlFilter.DoReplayGain"

/*----------------------------------------------------------------------
|   module
+---------------------------------------------------------------------*/
BLT_Result BLT_GainControlFilterModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_GAIN_CONTROL_FILTER_H_ */
