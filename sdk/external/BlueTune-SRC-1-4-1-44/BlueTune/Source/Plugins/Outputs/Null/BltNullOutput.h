/*****************************************************************
|
|   Null Output Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_NULL_OUTPUT_H_
#define _BLT_NULL_OUTPUT_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_output_modules
 * @defgroup null_output_module Null Output Module 
 * Plugin module that creates media nodes that can be used as an output. 
 * It produces media nodes that simply discard the media packets they
 * receive.
 * This module responds to probes with the name 'null'.
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
BLT_Result BLT_NullOutputModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_NULL_OUTPUT_H_ */
