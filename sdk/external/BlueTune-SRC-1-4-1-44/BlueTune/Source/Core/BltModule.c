/*****************************************************************
|
|   BlueTune - Module Interface
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "BltConfig.h"
#include "BltDefs.h"
#include "BltModule.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.core.module")

/*----------------------------------------------------------------------
|   forward references
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(BLT_BaseModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(BLT_BaseModule, ATX_Referenceable)

/*----------------------------------------------------------------------
|   BLT_BaseModule_Construct
+---------------------------------------------------------------------*/
BLT_Result
BLT_BaseModule_Construct(BLT_BaseModule* self, 
                         BLT_CString     name, 
                         BLT_CString     uid,
                         BLT_Flags       flags)
{
    return BLT_BaseModule_ConstructEx(self, name, uid, flags, 0, NULL);
}

/*----------------------------------------------------------------------
|   BLT_BaseModule_ConstructEx
+---------------------------------------------------------------------*/
BLT_Result
BLT_BaseModule_ConstructEx(BLT_BaseModule*     self, 
                           BLT_CString         name, 
                           BLT_CString         uid,
                           BLT_Flags           flags,
                           BLT_Cardinal        property_count,
                           const ATX_Property* properties)
{
    unsigned int i;
    if (name) {
        self->info.name = ATX_DuplicateString(name);
    } else {
        self->info.name = NULL;
    }
    if (uid) {
        self->info.uid = ATX_DuplicateString(uid);
    } else {
        self->info.uid = NULL;
    }
    self->info.flags      = flags;
    self->info.property_count  = property_count;
    if (property_count) {
        self->info.properties = (ATX_Property*)ATX_AllocateZeroMemory(property_count*sizeof(ATX_Property));
        for (i=0; i<property_count; i++) {
            ATX_Property_Clone(&properties[i], &self->info.properties[i]);
        }
    }
    
    self->reference_count = 1;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   BLT_BaseModule_Destruct
+---------------------------------------------------------------------*/
BLT_Result
BLT_BaseModule_Destruct(BLT_BaseModule* module)
{
    unsigned int i;
    if (module->info.name) {
        ATX_FreeMemory((void*)module->info.name);
    }
    if (module->info.uid) {
        ATX_FreeMemory((void*)module->info.uid);
    }
    for (i=0; i<module->info.property_count; i++) {
        ATX_Property_Destruct(&module->info.properties[i]);
    }
    if (module->info.properties) {
        ATX_FreeMemory(module->info.properties);
    }
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   BLT_BaseModule_Create
+---------------------------------------------------------------------*/
BLT_Result
BLT_BaseModule_Create(BLT_CString                name, 
                      BLT_CString                uid,
                      BLT_Flags                  flags,
                      const BLT_ModuleInterface* module_interface,
                      const ATX_ReferenceableInterface* referenceable_interface,
                      BLT_Module**               object)
{
    return BLT_BaseModule_CreateEx(name, 
                                   uid, 
                                   flags, 
                                   0, 
                                   NULL, 
                                   module_interface, 
                                   referenceable_interface, 
                                   0,
                                   object);
}

/*----------------------------------------------------------------------
|   BLT_BaseModule_CreateEx
+---------------------------------------------------------------------*/
BLT_Result
BLT_BaseModule_CreateEx(BLT_CString                name, 
                        BLT_CString                uid,
                        BLT_Flags                  flags,
                        BLT_Cardinal               property_count,
                        const ATX_Property*        properties,
                        const BLT_ModuleInterface* module_interface,
                        const ATX_ReferenceableInterface* referenceable_interface,
                        BLT_Size                   instance_size,
                        BLT_Module**               object)
{
    BLT_BaseModule* module;

    ATX_LOG_FINE_1("creating module name=%s", name);
    
    /* allocate memory for the object */
    if (instance_size == 0) instance_size = sizeof(BLT_BaseModule);
    module = (BLT_BaseModule*)ATX_AllocateZeroMemory(instance_size);
    if (module == NULL) {
        *object = NULL;
        return ATX_ERROR_OUT_OF_MEMORY;
    }

    /* construct the object */
    BLT_BaseModule_ConstructEx(module, name, uid, flags, property_count, properties);

    /* setup interfaces */
    ATX_BASE(module, BLT_Module).iface = module_interface;
    ATX_BASE(module, ATX_Referenceable).iface = referenceable_interface;
    *object = &ATX_BASE(module, BLT_Module);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   BLT_BaseModule_Destroy
+---------------------------------------------------------------------*/
BLT_Result
BLT_BaseModule_Destroy(BLT_BaseModule* module)
{
    ATX_LOG_FINE_1("destroying module name=%s", module->info.name);

    BLT_BaseModule_Destruct(module);
    ATX_FreeMemory((void*)module);
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   BLT_BaseModule_GetInfo
+---------------------------------------------------------------------*/
BLT_DIRECT_METHOD
BLT_BaseModule_GetInfo(BLT_Module* _self, BLT_ModuleInfo* info)
{
    BLT_BaseModule* self = ATX_SELF(BLT_BaseModule, BLT_Module);

    /* check parameters */
    if (info == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    /* return the module info */
    *info = self->info;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   BLT_BaseModule_Attach
+---------------------------------------------------------------------*/
BLT_DIRECT_METHOD
BLT_BaseModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    BLT_COMPILER_UNUSED(_self);
    BLT_COMPILER_UNUSED(core);
#if defined(BLT_DEBUG)
    {
        BLT_BaseModule* self = ATX_SELF(BLT_BaseModule, BLT_Module);
        ATX_LOG_FINE_1("attaching module name=%s", self->info.name?self->info.name:"");
    }
#endif
    return BLT_SUCCESS;
}

