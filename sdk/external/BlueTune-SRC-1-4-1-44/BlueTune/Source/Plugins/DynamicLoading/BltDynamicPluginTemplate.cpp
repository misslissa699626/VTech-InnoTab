/*****************************************************************
|
|   BlueTune - Dynamically Loaded Plugin Template
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "BltErrors.h"
#include "BltModule.h"
#include "BltVersion.h"
#include "BltDynamicPlugins.h"

/*----------------------------------------------------------------------
|    macro checking
+---------------------------------------------------------------------*/
#if !defined(BLT_PLUGIN_TEMPLATE_MODULE_FACTORY_FUNCTION)
#error BLT_PLUGIN_TEMPLATE_MODULE_FACTORY_FUNCTION not defined
#endif

/*----------------------------------------------------------------------
|    synthetic function prototypes
+---------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif
BLT_Result BLT_PLUGIN_TEMPLATE_MODULE_FACTORY_FUNCTION(BLT_Module** module);
BLT_Result BLT_PLUGIN_GET_MODULE_FUNCTION_NAME(BLT_UInt32   abi_version, 
                                               BLT_Ordinal  module_index,
                                               BLT_Module** module);
#if defined(__cplusplus)
}
#endif

/*----------------------------------------------------------------------
|    BLT_Plugin_GetModule
+---------------------------------------------------------------------*/
BLT_Result
BLT_PLUGIN_GET_MODULE_FUNCTION_NAME(BLT_UInt32   abi_version, 
                                    BLT_Ordinal  module_index,
                                    BLT_Module** module)
{
    if (module == NULL)                        return BLT_ERROR_INVALID_PARAMETERS;
    if (module_index != 0)                     return BLT_ERROR_NO_MATCHING_MODULE;
    if (abi_version != BLT_PLUGIN_ABI_VERSION) return ATX_ERROR_NOT_SUPPORTED;
    
    *module = NULL;
    
    return BLT_PLUGIN_TEMPLATE_MODULE_FACTORY_FUNCTION(module);
}

