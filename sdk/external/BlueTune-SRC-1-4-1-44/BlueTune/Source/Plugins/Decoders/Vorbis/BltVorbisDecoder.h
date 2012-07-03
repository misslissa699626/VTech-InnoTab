/*****************************************************************
|
|   Vorbis Decoder Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_VORBIS_DECODER_H_
#define _BLT_VORBIS_DECODER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_decoder_modules
 * @defgroup vorbis_decoder_module Vorbis Decoder Module 
 * Plugin module that creates media nodes capable of decoding Vorbis
 * audio. 
 * These media nodes expect a byte stream with ogg-vorbis encoded audio, 
 * and produce media packets with PCM audio.
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
BLT_Result BLT_VorbisDecoderModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_VORBIS_DECODER_H_ */
