/*****************************************************************
|
|   BlueTune - Core Object
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BlueTune Core
 */

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltTypes.h"
#include "BltDefs.h"
#include "BltErrors.h"
#include "BltModule.h"
#include "BltCore.h"
#include "BltStreamPriv.h"
#include "BltMediaNode.h"
#include "BltRegistryPriv.h"
#include "BltMediaPacketPriv.h"
#include "BltCorePriv.h"
#include "BltPcm.h"

/*----------------------------------------------------------------------
|    logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.core")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_Core);
    ATX_IMPLEMENTS(ATX_Destroyable);

    /* members */
    BLT_Registry*   registry;
    ATX_Properties* properties;
    ATX_List*       modules;
} Core;

/*----------------------------------------------------------------------
|    forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(Core, BLT_Core)
ATX_DECLARE_INTERFACE_MAP(Core, ATX_Destroyable)

/*----------------------------------------------------------------------
|    Core_Create
+---------------------------------------------------------------------*/
static BLT_Result
Core_Create(BLT_Core** object)
{
    Core*      core;
    BLT_Result result;

    ATX_LOG_FINE("Core::Create");
    
    /* allocate memory for the object */
    core = ATX_AllocateZeroMemory(sizeof(Core));
    if (core == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* create the registry */
    result = Registry_Create(&core->registry);
    if (BLT_FAILED(result)) {
        *object = NULL;
        ATX_FreeMemory(core);
        return result;
    }

    /* create the properties */
    ATX_Properties_Create(&core->properties);

    /* create the module list */
    result = ATX_List_Create(&core->modules);
    if (BLT_FAILED(result)) {
        ATX_DESTROY_OBJECT(core->registry);
        *object = NULL;
        ATX_FreeMemory(core);
        return result;
    }

    /* setup interfaces */
    ATX_SET_INTERFACE(core, Core, BLT_Core);
    ATX_SET_INTERFACE(core, Core, ATX_Destroyable);
    *object = &ATX_BASE(core, BLT_Core);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Core_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
Core_Destroy(ATX_Destroyable* _self)
{
    Core* core = ATX_SELF(Core, ATX_Destroyable);
    ATX_ListItem* item = ATX_List_GetFirstItem(core->modules);
    
    ATX_LOG_FINE("Core::Destroy");
    
    /* release the modules in the list */
    while (item) {
        BLT_Module* module = (BLT_Module*)ATX_ListItem_GetData(item);
        ATX_RELEASE_OBJECT(module);
        item = ATX_ListItem_GetNext(item);
    }

    /* delete the module list */
    ATX_List_Destroy(core->modules);

    /* destroy the properties */
    ATX_DESTROY_OBJECT(core->properties);

    /* destroy the registry */
    BLT_Registry_Destroy(core->registry);

    /* free the object memory */
    ATX_FreeMemory((void*)core);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Core_CreateStream
+---------------------------------------------------------------------*/
BLT_METHOD 
Core_CreateStream(BLT_Core* self, BLT_Stream** stream)
{
    /* create a stream and return */
    return Stream_Create(self, stream);
}

/*----------------------------------------------------------------------
|    Core_RegisterModule
+---------------------------------------------------------------------*/
BLT_METHOD 
Core_RegisterModule(BLT_Core* _self, BLT_Module* module)
{
    Core*      self = ATX_SELF(Core, BLT_Core);
    BLT_Result result;

    /* add the module object to the list */
    result = ATX_List_AddData(self->modules, (ATX_Object*)module);
    if (BLT_FAILED(result)) return result;

    /* keep a reference to the object */
    ATX_REFERENCE_OBJECT(module);
    
    /* attach the module to the core */
    result = BLT_Module_Attach(module, _self);
    if (BLT_FAILED(result)) return result;
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Core_UnRegisterModule
+---------------------------------------------------------------------*/
BLT_METHOD 
Core_UnRegisterModule(BLT_Core* _self, BLT_Module* module)
{
    Core* self = ATX_SELF(Core, BLT_Core);

    /* release the reference */
    ATX_RELEASE_OBJECT(module);
    
    /* remove the module object from the list */
    return ATX_List_RemoveData(self->modules, (ATX_Object*)module);
}

/*----------------------------------------------------------------------
|    Core_EnumerateModules
+---------------------------------------------------------------------*/
BLT_METHOD
Core_EnumerateModules(BLT_Core* _self, ATX_List** modules)
{
    Core*         self = ATX_SELF(Core, BLT_Core);
    ATX_ListItem* item;
    BLT_Result    result;
    
    /* create the list */
    *modules = NULL;
    result = ATX_List_Create(modules);
    if (BLT_FAILED(result)) return result;
    
    /* populate the list */
    for (item = ATX_List_GetFirstItem(self->modules);
         item;
         item = ATX_ListItem_GetNext(item)) {
        ATX_List_AddData(*modules, ATX_ListItem_GetData(item));
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Core_GetRegistry
+---------------------------------------------------------------------*/
BLT_METHOD
Core_GetRegistry(BLT_Core* _self, BLT_Registry** registry)
{
    Core* self = ATX_SELF(Core, BLT_Core);
    *registry = self->registry;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Core_GetProperties
+---------------------------------------------------------------------*/
BLT_METHOD
Core_GetProperties(BLT_Core* _self, ATX_Properties** properties)
{
    Core* self = ATX_SELF(Core, BLT_Core);
    *properties = self->properties;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Core_CreateCompatibleNode
+---------------------------------------------------------------------*/
BLT_METHOD
Core_CreateCompatibleNode(BLT_Core*                 _self, 
                          BLT_MediaNodeConstructor* constructor,
                          BLT_MediaNode**           node)
{
    Core*         core        = ATX_SELF(Core, BLT_Core);
    ATX_ListItem* item        = ATX_List_GetFirstItem(core->modules);
    int           best_match  = -1;
    BLT_Module*   best_module = NULL;
            
    /* find a module that responds to the probe */
    while (item) {
        BLT_Result   result;
        BLT_Module*  module;
        BLT_Cardinal match;

        /* get the module object from the list */
        module = (BLT_Module*)ATX_ListItem_GetData(item);

        /* probe the module */
        result = BLT_Module_Probe(
            module, 
            _self,
            BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR,
            constructor,
            &match);

        /* keep it if it is a better match than the others */
        if (BLT_SUCCEEDED(result)) {
            if ((int)match > best_match) {
                best_match  = match;
                best_module = module;
            }
        }

        /* move on to the next module */
        item = ATX_ListItem_GetNext(item);
    }

    if (best_match == -1) {
        /* no matching module found */
        return BLT_ERROR_NO_MATCHING_MODULE;
    }

    /* create a node instance */
    return BLT_Module_CreateInstance(
        best_module, 
        _self, 
        BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR, 
        constructor,
        &ATX_INTERFACE_ID__BLT_MediaNode,
        (ATX_Object**)node);
}

/*----------------------------------------------------------------------
|    Core_CreateMediaPacket
+---------------------------------------------------------------------*/
BLT_METHOD
Core_CreateMediaPacket(BLT_Core*            self,
                       BLT_Size             size,
                       const BLT_MediaType* type,
                       BLT_MediaPacket**    packet)
{       
    BLT_COMPILER_UNUSED(self);
    return BLT_MediaPacket_Create(size, type, packet);
}

/*----------------------------------------------------------------------
|    Core_ParseMimeType
|
|    NOTE: this function has built-in knowlege of some data types, 
|    but it shouldn't. This should be pluggable. It will be in 
|    a future version of the library.
+---------------------------------------------------------------------*/
BLT_METHOD 
Core_ParseMimeType(BLT_Core*       _self, 
                   const char*     mime_type, 
                   BLT_MediaType** media_type)
{
    Core*           self = ATX_SELF(Core, BLT_Core);
    BLT_Result      result = ATX_SUCCESS;
    BLT_MediaTypeId media_type_id = 0;
    ATX_String      workspace = ATX_EMPTY_STRING;
    
    /* default */
    *media_type = NULL;
    
    /* see if the mime type has parameters */
    ATX_String_Assign(&workspace, mime_type);
    {
        int sep = ATX_String_FindChar(&workspace, ';');
        if (sep >= 0) {
            ATX_String_SetLength(&workspace, sep);
            ATX_String_TrimWhitespaceRight(&workspace);
        }
    }
    
    /* look for known types */
    if (ATX_String_Equals(&workspace, "audio/L16", ATX_TRUE)) {
        result = BLT_Pcm_ParseMimeType(mime_type, (BLT_PcmMediaType**)media_type);
    } else {
        result = BLT_Registry_GetIdForName(self->registry,
                                           BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS, 
                                           ATX_CSTR(workspace), 
                                           &media_type_id);
        if (ATX_SUCCEEDED(result)) {
            if (media_type_id == BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
                BLT_PcmMediaType* pcm_media_type = ATX_AllocateZeroMemory(sizeof(BLT_PcmMediaType));
                BLT_PcmMediaType_Init(pcm_media_type);
                *media_type = (BLT_MediaType*)pcm_media_type;
            } else {
                *media_type = ATX_AllocateZeroMemory(sizeof(BLT_MediaType));
                (*media_type)->id = media_type_id;
            }
        }
    }
    
    ATX_String_Destruct(&workspace);
    return result;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(Core)
    ATX_GET_INTERFACE_ACCEPT(Core, BLT_Core)
    ATX_GET_INTERFACE_ACCEPT(Core, ATX_Destroyable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_Core interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(Core, BLT_Core)
    Core_CreateStream,
    Core_RegisterModule,
    Core_UnRegisterModule,
    Core_EnumerateModules,
    Core_GetRegistry,
    Core_GetProperties,
    Core_CreateCompatibleNode,
    Core_CreateMediaPacket,
    Core_ParseMimeType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_DESTROYABLE_INTERFACE(Core)

/*----------------------------------------------------------------------
|    BLT_Core_Create
+---------------------------------------------------------------------*/
BLT_Result
BLT_Core_Create(BLT_Core** core)
{
    return Core_Create(core);
}

