/*****************************************************************
|
|   Fluo - Configuration
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Fluo Configuration
 */

#ifndef _FLO_CONFIG_H_
#define _FLO_CONFIG_H_

/*----------------------------------------------------------------------
|   decoder engine selection
+---------------------------------------------------------------------*/
#define FLO_DECODER_ENGINE_BUILTIN 0
#define FLO_DECODER_ENGINE_MPG123  1
#define FLO_DECODER_ENGINE_FFMPEG  2

#if !defined(FLO_CONFIG_DECODER_ENGINE)
#define FLO_CONFIG_DECODER_ENGINE FLO_DECODER_ENGINE_BUILTIN
#endif

/*----------------------------------------------------------------------
|   defaults
+---------------------------------------------------------------------*/
#define FLO_CONFIG_HAVE_STDLIB_H

#define FLO_CONFIG_HAVE_CALLOC
#define FLO_CONFIG_HAVE_MALLOC
#define FLO_CONFIG_HAVE_FREE
#define FLO_CONFIG_HAVE_MEMCPY
#define FLO_CONFIG_HAVE_MEMSET
#define FLO_CONFIG_HAVE_MEMMOVE

/*----------------------------------------------------------------------
|   compiler specifics
+---------------------------------------------------------------------*/
#if defined(_MSC_VER)
#define inline __inline
#endif

#endif /* _FLO_CONFIG_H_ */
