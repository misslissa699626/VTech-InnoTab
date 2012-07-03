/*****************************************************************
|
|   PCM Adapter Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_PCM_ADAPTER_H_
#define _BLT_PCM_ADAPTER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_adapter_modules
 * @defgroup pcm_adapter_module PCM Adapter Module 
 * Plugin module that creates media nodes that adapt media packets with
 * PCM audio into media packets with PCM audio with different parameters.
 * This module is typically used by the stream manager to adapt the PCM audio
 * format between a media node that produces media packets with PCM audio that
 * needs to be transformed before it can be consumed by another media node.
 * This includes, for example, going from single channel to stereo, or from 
 * 24-bit samples to 16-bit samples, or floating point samples to integer samples.
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
BLT_Result BLT_PcmAdapterModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_PCM_ADAPTER_H_ */
