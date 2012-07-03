/*****************************************************************
|
|   Debug Output Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_DEBUG_OUTPUT_H_
#define _BLT_DEBUG_OUTPUT_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_output_modules
 * @defgroup debug_output_module Debug Output Module 
 * Plugin module that creates media nodes that can be used as an output. 
 * It produces media nodes that log details about the media packets 
 * they receive, and then discard them.
 * This module responds to probes with the name 'debug'.
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
BLT_Result BLT_DebugOutputModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_DEBUG_OUTPUT_H_ */
