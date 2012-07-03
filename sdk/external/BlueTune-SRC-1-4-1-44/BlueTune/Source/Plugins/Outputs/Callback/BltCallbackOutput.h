/*****************************************************************
|
|   Callback Output Module
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_CALLBACK_OUTPUT_H_
#define _BLT_CALLBACK_OUTPUT_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_output_modules
 * @defgroup callback_output_module Callback Output Module 
 * Plugin module that creates media nodes that can be used as an output. 
 * It produces media nodes that call back an object for each received packet.
 * This module responds to probes with the name 'callback-output:<addr>',
 * where <addr> is a decimal representation of the address of an object that
 * implements the BLT_PacketConsumer interface.
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
    
BLT_Result BLT_CallbackOutputModule_GetModuleObject(BLT_Module** module);

#if defined(__cplusplus)
}
#endif

/** @} */

#endif /* _BLT_CALLBACK_OUTPUT_H_ */
