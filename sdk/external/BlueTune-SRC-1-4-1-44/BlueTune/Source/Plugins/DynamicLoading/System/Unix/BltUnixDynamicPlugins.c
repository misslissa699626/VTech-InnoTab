/*****************************************************************
|
|   BlueTune - Dynamically Loaded Plugins :: Unix Support
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltDynamicPlugins.h"

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
BLT_Result 
BLT_Plugins_GetDefaultPluginsDirectory(ATX_String* path)
{
    ATX_Result result;
    
    if (path == NULL) return BLT_ERROR_INVALID_PARAMETERS;
    result = ATX_GetEnvironment("HOME", path);
    if (ATX_FAILED(result)) {
        ATX_String_SetLength(path, 0);
        return BLT_ERROR_NOT_SUPPORTED;
    }
    ATX_String_Append(path, "/.bluetune/plugins");
    
    return BLT_SUCCESS;
}

