/*****************************************************************
|
|   Intel IPP Decoder Module
|
|   (c) 2008-2009 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_IPP_DECODER_H_
#define _BLT_IPP_DECODER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_decoder_modules
 * @defgroup ipp_decoder_module Intel IPP Decoder Module 
 * Plugin module creates media nodes capable of decoding the different 
 * media types supported by the Intel IPP library.
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

BLT_Result BLT_IppDecoderModule_GetModuleObject(BLT_Module** module);

#if defined(__cplusplus)
}
#endif

#endif /* _BLT_IPP_DECODER_H_ */
