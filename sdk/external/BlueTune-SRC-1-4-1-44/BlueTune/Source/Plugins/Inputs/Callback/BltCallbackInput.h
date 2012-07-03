/*****************************************************************
|
|   Callback Input Module
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_CALLBACK_INPUT_H_
#define _BLT_CALLBACK_INPUT_H_

/**
 * @ingroup plugin_modules
 * @defgroup callback_input_module Callback Input Module 
 * Plugin module that creates media nodes that read files.
 * This module will respond to probes with names that have the 
 * following syntax: 
 * callback-input:<addr-of-input-stream-object>
 * where <addr-of-input-stream-object> is the address of a BLT_InputStream
 * object represented as a decimal number.
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

BLT_Result BLT_CallbackInputModule_GetModuleObject(BLT_Module** module);

#if defined(__cplusplus)
}
#endif

/** @} */

#endif /* _BLT_CALLBACK_INPUT_H_ */
