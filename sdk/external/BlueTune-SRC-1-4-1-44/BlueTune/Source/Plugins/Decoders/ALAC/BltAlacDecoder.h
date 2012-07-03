/*****************************************************************
|
|   ALAC Decoder Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_ALAC_DECODER_H_
#define _BLT_ALAC_DECODER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_decoder_modules
 * @defgroup alac_decoder_module ALAC Decoder Module 
 * Plugin module that creates media nodes capable of decoding ALAC
 * audio (Apple Lossless Audio Codec). 
 * These media nodes expect media buffers with ALAC audio frames
 * and produces media packets with PCM audio.
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
/**
 * Returns a pointer to the ALAC Decoder module.
 */
BLT_Result BLT_AlacDecoderModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_ALAC_DECODER_H_ */
