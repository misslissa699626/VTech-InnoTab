/*****************************************************************
|
|   MPEG Audio Decoder Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_MPEG_AUDIO_DECODER_H_
#define _BLT_MPEG_AUDIO_DECODER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_decoder_modules
 * @defgroup mpeg_audio_decoder_module MPEG Audio Decoder Module 
 * Plugin module creates media nodes capable of decoding MPEG1 
 * and MPEG2 layers 1, 2 and 3 (MP3) compressed audio.
 * These media nodes expect media buffers with MPEG audio data without
 * any special framing. They produce media buffers with PCM audio.
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
BLT_Result BLT_MpegAudioDecoderModule_GetModuleObject(BLT_Module** module);

#endif /* _BLT_MPEG_AUDIO_DECODER_H_ */
