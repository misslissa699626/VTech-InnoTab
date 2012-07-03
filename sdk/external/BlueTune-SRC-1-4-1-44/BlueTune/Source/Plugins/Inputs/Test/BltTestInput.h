/*****************************************************************
|
|   Test Input Module
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_TEST_INPUT_H_
#define _BLT_TEST_INPUT_H_

/**
 * @ingroup plugin_modules
 * @defgroup file_input_module Test Input Module 
 * Plugin module that creates media nodes that produce synthetic
 * content for testing.
 * This module will respond to probes with names that have the 
 * following syntax: 
 * test:[video|pcm]
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
BLT_Result BLT_TestInputModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_TEST_INPUT_H_ */
