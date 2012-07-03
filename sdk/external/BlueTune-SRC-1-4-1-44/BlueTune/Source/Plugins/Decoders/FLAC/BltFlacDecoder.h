/*****************************************************************
|
|   Flac Decoder Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_FLAC_DECODER_H_
#define _BLT_FLAC_DECODER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_decoder_modules
 * @defgroup flac_decoder_module FLAC Decoder Module 
 * Plugin module that create media nodes capable of decoding FLAC audio. 
 * These media nodes expect an input byte stream with FLAC-encoded
 * audio data. They produce media packets with PCM audio.
 * More information about the FLAC format can be found at:
 * http://flac.sourceforge.net
 *
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
BLT_Result BLT_FlacDecoderModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_FLAC_DECODER_H_ */
