/*****************************************************************
|
|   DcfParser Module
|
|   (c) 2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_DCF_PARSER_H_
#define _BLT_DCF_PARSER_H_

/**
 * @ingroup plugin_modules
 * @ingroup plugin_parser_modules
 * @defgroup dcf_parser_module DCF Parser Module 
 * Plugin module that creates media nodes that parse the OMA DCF file format. 
 * These media nodes expect a byte stream input with DCF formatted data.
 * They produce a typed stream as output.
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

BLT_Result BLT_DcfParserModule_GetModuleObject(BLT_Module** module);

#if defined(__cplusplus)
}
#endif

/** @} */

#endif /* _BLT_DCF_PARSER_H_ */
