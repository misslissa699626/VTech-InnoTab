/*****************************************************************
|
|   Windows Media Services Protocol Module
|
|   (c) 2002-2010 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_WMS_PROTOCOL_H_
#define _BLT_WMS_PROTOCOL_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_protocol_modules
 * @defgroup wms_protocol_module Microsoft Windows Media Services Protocol Module 
 * Plugin module that creates media nodes that can read from ASX streams. 
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
#if defined(__cplusplus) 
extern "C" {
#endif

BLT_Result BLT_WmsProtocolModule_GetModuleObject(BLT_Module** module);

#if defined(__cplusplus)
}
#endif

/** @} */

#endif /* _BLT_WMS_PROTOCOL_H_ */
