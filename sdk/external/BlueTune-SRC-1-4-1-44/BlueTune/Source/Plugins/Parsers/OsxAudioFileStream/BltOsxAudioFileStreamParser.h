/*****************************************************************
|
|   OSX AudioFileStream Parser Module
|
|   (c) 2002-2010 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_OSX_AUDIO_FILE_STREAM_PARSER_H_
#define _BLT_OSX_AUDIO_FILE_STREAM_PARSER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_decoder_modules
 * @defgroup osx_audio_file_stream_decoder_module OSX AudioFileStream Parser Module 
 * Plugin module that creates media nodes capable of parsing audio streams
 * using OSX's AudioFileStream API. 
 * These media nodes expect media packets with audio data in any of the 
 * supported formats and produces media packets with the parsed compressed
 * audio.
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
 * Returns a pointer to the OSX AudioFileStream Parser module.
 */
BLT_Result BLT_OsxAudioFileStreamParserModule_GetModuleObject(BLT_Module** module);

/** @} */

#endif /* _BLT_OSX_AUDIO_FILE_STREAM_PARSER_H_ */
