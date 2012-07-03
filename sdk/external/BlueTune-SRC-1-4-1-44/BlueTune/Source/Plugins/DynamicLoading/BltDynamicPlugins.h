/*****************************************************************
|
|   BlueTune - Dynamically Loaded Plugins
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_DYNAMIC_PLUGIN_H_
#define _BLT_DYNAMIC_PLUGIN_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "BltErrors.h"
#include "BltModule.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_PLUGIN_GET_MODULE_FUNCTION_NAME         BLT_Plugin_GetModule
#define BLT_PLUGIN_GET_MODULE_FUNCTION_NAME_STRING "BLT_Plugin_GetModule"

#define BLT_PLUGIN_LOADER_FLAGS_SEARCH_OS_LOADER_PATH      1
#define BLT_PLUGIN_LOADER_FLAGS_SEARCH_WORKING_DIRECTORY   2
#define BLT_PLUGIN_LOADER_FLAGS_SEARCH_DEFAULT_DIRECTORIES 4
#define BLT_PLUGIN_LOADER_FLAGS_SEARCH_SPECIAL_NAMES       8
#define BLT_PLUGIN_LOADER_FLAGS_SEARCH_ALL                 0xFFFFFFFF

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef BLT_Result (*BLT_Plugin_GetModule_Function)(
    BLT_UInt32   abi_version, 
    BLT_Ordinal  module_index,
    BLT_Module** module);

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif

BLT_Result BLT_Plugins_GetDefaultPluginsDirectory(ATX_String* path);
BLT_Result BLT_Plugins_LoadModulesFromFile(BLT_Core*   core, 
                                           const char* name, 
                                           BLT_Flags   search_flags);
BLT_Result BLT_Plugins_LoadModulesFromDirectory(BLT_Core*   core, 
                                                const char* directory,
                                                const char* file_extension);

#if defined(__cplusplus)
}
#endif
                     
#endif /* _BLT_DYNAMIC_PLUGIN_H_ */

