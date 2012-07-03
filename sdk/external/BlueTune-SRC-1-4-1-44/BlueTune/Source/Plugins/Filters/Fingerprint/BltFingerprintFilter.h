/*****************************************************************
|
|   Fingerprint Filter Module
|
|   (c) 2002-2010 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_FINGERPRINT_FILTER_H_
#define _BLT_FINGERPRINT_FILTER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_filter_modules
 * @defgroup fingerprint_filter_module Fingerprint Filter Module 
 * Plugin module that computes a fingerprint of the PCM audio data
 * that passes through it.
 * These media nodes expect media packets with PCM audio as input, 
 * and produce media packets with PCM audio as output.
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
#define BLT_FINGERPRINT_FILTER_MODE "plugins.filters.fingerprint.mode"

/*----------------------------------------------------------------------
|   module
+---------------------------------------------------------------------*/
BLT_Result BLT_FingerprintFilterModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_FINGERPRINT_FILTER_H_ */
