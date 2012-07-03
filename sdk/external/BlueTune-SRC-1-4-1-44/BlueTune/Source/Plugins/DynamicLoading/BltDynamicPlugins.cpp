/*****************************************************************
|
|   BlueTune - Dynamically Loaded Plugins
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "BltErrors.h"
#include "BltModule.h"
#include "BltDynamicPlugins.h"
#include "BltVersion.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.loader")

/*----------------------------------------------------------------------
|   BLT_Plugins_LoadModulesFromLibrary
+---------------------------------------------------------------------*/
static BLT_Result 
BLT_Plugins_LoadModulesFromLibrary(BLT_Core* core, const char* path)
{
    NPT_DynamicLibrary* library = NULL;
    NPT_Result          result;
    
    // load the library
    result = NPT_DynamicLibrary::Load(path, 
                                      NPT_DYANMIC_LIBRARY_LOAD_FLAG_NOW,
                                      library);
    if (NPT_FAILED(result)) return result;

    // find the entry point symbol
    void* symbol = NULL;
    result = library->FindSymbol(BLT_PLUGIN_GET_MODULE_FUNCTION_NAME_STRING, symbol);
    if (NPT_FAILED(result)) {
        ATX_LOG_FINE("symbol not found");
        return result;
    }
    
    // call the function for each module in the library
    union {
        BLT_Plugin_GetModule_Function function;
        void*                         symbol;
    } function_symbol;
    function_symbol.symbol = symbol;
    BLT_Plugin_GetModule_Function function = function_symbol.function;
    for (unsigned int i=0; BLT_SUCCEEDED(result); ++i) {
        BLT_Module* module = NULL;
        result = function(BLT_PLUGIN_ABI_VERSION, i, &module);
        if (BLT_FAILED(result)) {
            ATX_LOG_FINE_1("BLT_Plugin_GetModule function returned %d", result);
            break;
        }
        ATX_LOG_FINE("registering module");
        BLT_Core_RegisterModule(core, module);
        ATX_RELEASE_OBJECT(module);
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   BLT_Plugins_LoadModulesFromFile
+---------------------------------------------------------------------*/
BLT_Result 
BLT_Plugins_LoadModulesFromFile(BLT_Core* core, const char* name, BLT_Flags search_flags)
{
    BLT_Result result;
    
    /* check args */
    if (name == NULL) return BLT_ERROR_INVALID_PARAMETERS;
    
    ATX_LOG_FINE_2("loading plugin module %s, flags=%x", name, search_flags);
    
    if (search_flags == 0) {
        ATX_LOG_FINE("loading module without searching");
        return BLT_Plugins_LoadModulesFromLibrary(core, name);
    }
    if (search_flags & BLT_PLUGIN_LOADER_FLAGS_SEARCH_WORKING_DIRECTORY) {
        ATX_LOG_FINE("searching working directory");
        NPT_String working_directory;
        result = NPT_File::GetWorkingDir(working_directory);
        if (NPT_SUCCEEDED(result)) {
            NPT_String path = working_directory + NPT_FilePath::Separator + name;
            result = BLT_Plugins_LoadModulesFromLibrary(core, path);
            if (BLT_SUCCEEDED(result)) return BLT_SUCCESS;
        } else {
            ATX_LOG_FINE("cannot get working directory, ignoring");
        }
    }
    if (search_flags & BLT_PLUGIN_LOADER_FLAGS_SEARCH_OS_LOADER_PATH) {
        ATX_LOG_FINE("searching os loader path");
        result = BLT_Plugins_LoadModulesFromLibrary(core, name);
        if (BLT_SUCCEEDED(result)) return BLT_SUCCESS;
    }
    
    ATX_LOG_FINE("no matching module");
    return BLT_ERROR_NO_MATCHING_MODULE;
}

/*----------------------------------------------------------------------
|   BLT_Plugins_LoadModulesFromDirectory
+---------------------------------------------------------------------*/
BLT_Result 
BLT_Plugins_LoadModulesFromDirectory(BLT_Core*   core, 
                                     const char* directory, 
                                     const char* file_extension)
{
    /* check args */
    if (directory == NULL) return BLT_ERROR_INVALID_PARAMETERS;
    
    ATX_LOG_FINE_1("loading modules in directory %s", directory);
    
    NPT_List<NPT_String> entries;
    NPT_Result result = NPT_File::ListDir(directory, entries);
    ATX_CHECK_FINE(result);

    for (NPT_List<NPT_String>::Iterator i = entries.GetFirstItem(); i; ++i) {
        // skip entries that do not have the required file extension
        if (file_extension && !(i->EndsWith(file_extension))) continue;
        
        // load the library
        NPT_String path = directory;
        path += NPT_FilePath::Separator;
        path += *i;
        BLT_Plugins_LoadModulesFromFile(core, path, 0);
    }

    return BLT_SUCCESS;
}

