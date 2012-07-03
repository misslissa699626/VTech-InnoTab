/*****************************************************************
|
|   BlueTune - Module Interface
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file 
 * BLT_Module interface 
 */

#ifndef _BLT_MODULE_H_
#define _BLT_MODULE_H_

/**
 * @defgroup plugin_modules Plugin Modules
 */

/**
 * @defgroup BLT_Module_interface BLT_Module Interface
 * Interface implemented by objects that create other objects.
 *  
 * A Module object is responsible for creating object instance of a certain 
 * class. Module objects implement the BLT_Module interface, and clients that
 * want to create instances of that module will call the CreateObject method.
 * @{
 */

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltTypes.h"
#include "BltCore.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
/**
 * Type identifiers for the parameters argument of the Probe method of
 * the BLT_Module interface.
 */
typedef enum {
    /**
     * The parameters pointer points to a BLT_MediaNodeConstructor.
     */
    BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR
} BLT_ModuleParametersType;

/**
 * Information about a module.
 */
typedef struct {
    BLT_CString   name;
    BLT_CString   uid;
    BLT_Flags     flags;
    BLT_Cardinal  property_count;
    ATX_Property* properties;
} BLT_ModuleInfo;

/**
 * Base implementation of the BLT_Module interface that other 
 * implementation of that interface can inherit from.
 */
typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_Module);
    ATX_IMPLEMENTS(ATX_Referenceable);

    /* members */
    BLT_Cardinal   reference_count;
    BLT_ModuleInfo info;
} BLT_BaseModule;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_MODULE_PROBE_MATCH_DEFAULT 0
#define BLT_MODULE_PROBE_MATCH_MIN     1
#define BLT_MODULE_PROBE_MATCH_LOW     64
#define BLT_MODULE_PROBE_MATCH_MEDIUM  128
#define BLT_MODULE_PROBE_MATCH_HIGH    192
#define BLT_MODULE_PROBE_MATCH_MAX     253
#define BLT_MODULE_PROBE_MATCH_EXACT   254
#define BLT_MODULE_PROBE_MATCH_FORCE   255

#define BLT_MODULE_FLAG_MEDIA_NODE_FACTORY 1

/*----------------------------------------------------------------------
|   error codes
+---------------------------------------------------------------------*/
#define BLT_ERROR_NO_MATCHING_MODULE (BLT_ERROR_BASE_MODULE - 0)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_DEFINITION(BLT_Module)
    /**
     * Get the module's info.
     * @param self Pointer to the BLT_Module object on which the method 
     * is called 
     * @param info Pointer to a BLT_ModuleInfo structure in which the
     * module's info will be returned.
     */
    BLT_Result (*GetInfo)(BLT_Module* self, BLT_ModuleInfo* info);

    /**
     * Attach the module to a BLT_Core object. The BLT_Core object reprents
     * the context in which the module is running. This allows the module, 
     * amongst other things, to access the core's registry.
     * @param self Pointer to the BLT_Module object on which the method 
     * is called 
     * @param core Pointer the BLT_Core object to which this module is being
     * attached.
     */
    BLT_Result (*Attach)(BLT_Module* self, BLT_Core* core);

    /** 
     * Create an instance of the module that implements a given interface
     * @param self Pointer to the BLT_Module object on which the method 
     * is called 
     * @param parameters Generic parameters used for constructing the object
     * @param interface_id Interface ID that the object needs to implement
     * @param object address of an object reference where the created object
     * will be returned if the call succeeds.
     */
    BLT_Result (*CreateInstance)(BLT_Module*              self,
                                 BLT_Core*                core,
                                 BLT_ModuleParametersType parameters_type,
                                 BLT_AnyConst             parameters,
                                 const ATX_InterfaceId*   interface_id,
                                 ATX_Object**             object);

    /**
     * Probe the module to know if it is able to create an oject instance
     * that can handle a certain task.
     * @param self Pointer to the BLT_Module object on which the method 
     * is called 
     * @param core Pointer to the BLT_Core object that represents the
     * core context for the call.
     * @param parameters_type Type identifier that indicates which specific
     * structure the parameters point to.
     * @param parameters Pointer to a parameters structure. The type of the
     * structure pointed to is indicated by the parameters_type parameter.
     * The type of parameters passed to this method indicates what type of
     * query is being made and what the query parameters are.
     */
    BLT_Result (*Probe)(BLT_Module*              self,
                        BLT_Core*                core,
                        BLT_ModuleParametersType parameters_type,
                        BLT_AnyConst             parameters,
                        BLT_Cardinal*            match);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define BLT_Module_GetInfo(object, info) \
ATX_INTERFACE(object)->GetInfo(object, info)

#define BLT_Module_Attach(object, core) \
ATX_INTERFACE(object)->Attach(object, core)

#define BLT_Module_CreateInstance(object, core, parameters_type, parameters, interface_id, new_object) \
ATX_INTERFACE(object)->CreateInstance(object,               \
                                      core,                 \
                                      parameters_type,      \
                                      parameters,           \
                                      interface_id,         \
                                      new_object)

#define BLT_Module_Probe(object, core, type, query, match) \
ATX_INTERFACE(object)->Probe(object, core, type, query, match)

/*----------------------------------------------------------------------
|   base methods
+---------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif

BLT_Result
BLT_BaseModule_Construct(BLT_BaseModule* self, 
                         BLT_CString     name, 
                         BLT_CString     uid,
                         BLT_Flags       flags);

BLT_Result
BLT_BaseModule_ConstructEx(BLT_BaseModule*     self, 
                           BLT_CString         name, 
                           BLT_CString         uid,
                           BLT_Flags           flags,
                           BLT_Cardinal        property_count,
                           const ATX_Property* properties);

BLT_Result
BLT_BaseModule_Destruct(BLT_BaseModule* self);

BLT_Result
BLT_BaseModule_Create(BLT_CString                       name, 
                      BLT_CString                       uid,
                      BLT_Flags                         flags,
                      const BLT_ModuleInterface*        module_interface,
                      const ATX_ReferenceableInterface* referenceable_interface,
                      BLT_Module**                      object);

BLT_Result
BLT_BaseModule_CreateEx(BLT_CString                       name, 
                        BLT_CString                       uid,
                        BLT_Flags                         flags,
                        BLT_Cardinal                      property_count,
                        const ATX_Property*               properties,
                        const BLT_ModuleInterface*        module_interface,
                        const ATX_ReferenceableInterface* referenceable_interface,
                        BLT_Size                          intance_size,
                        BLT_Module**                      object);

BLT_Result
BLT_BaseModule_Destroy(BLT_BaseModule* self);

BLT_DIRECT_METHOD
BLT_BaseModule_GetInfo(BLT_Module* self, BLT_ModuleInfo* info);

BLT_DIRECT_METHOD
BLT_BaseModule_Attach(BLT_Module* self, BLT_Core* core);

#if defined (__cplusplus)
}
#endif

/*----------------------------------------------------------------------
|   template macros
+---------------------------------------------------------------------*/
#define BLT_MODULE_IMPLEMENT_SIMPLE_CONSTRUCTOR(_module_class,      \
                                                _module_name,       \
                                                _module_flags)      \
static BLT_Result                                                   \
_module_class##_Create(BLT_Module** object)                         \
{                                                                   \
    _module_class* module;                                          \
                                                                    \
    /* allocate memory for the object */                            \
    module = (_module_class*)                                       \
        ATX_AllocateZeroMemory(sizeof(_module_class));              \
                                                                    \
    /* construct the inherited object */                            \
    BLT_BaseModule_Construct(&ATX_BASE(module, BLT_BaseModule),     \
                             _module_name,                          \
                             NULL,                                  \
                             _module_flags);                        \
                                                                    \
    /* setup interfaces */                                          \
    ATX_SET_INTERFACE_EX(module, _module_class,                     \
                        BLT_BaseModule, BLT_Module);                \
    ATX_SET_INTERFACE_EX(module, _module_class,                     \
                         BLT_BaseModule, ATX_Referenceable);        \
    *object = &ATX_BASE_EX(module, BLT_BaseModule, BLT_Module);     \
    return BLT_SUCCESS;                                             \
}

#define BLT_MODULE_AXIOMATIC_COPYRIGHT "(c) 2001-2010 Axiomatic Systems, LLC"
#define BLT_MODULE_DECLARE_STANDARD_PROPERTIES(_version, _copyright) \
ATX_Property properties[2] = {                                       \
    {"version",   {ATX_PROPERTY_VALUE_TYPE_STRING, {_version}}},     \
    {"copyright", {ATX_PROPERTY_VALUE_TYPE_STRING, {_copyright}}}    \
}
#define BLT_MODULE_ARGS_STANDARD_PROPERTIES ATX_ARRAY_SIZE(properties), properties

#define BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(_class, _name, _uid, _version, _copyright) \
BLT_Result BLT_##_class##_GetModuleObject(BLT_Module** object)                  \
{                                                                               \
    BLT_MODULE_DECLARE_STANDARD_PROPERTIES(_version, _copyright);               \
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;                    \
                                                                                \
    return BLT_BaseModule_CreateEx(_name,                                       \
                                   _uid,                                        \
                                   0,                                           \
                                   BLT_MODULE_ARGS_STANDARD_PROPERTIES,         \
                                   &_class##_BLT_ModuleInterface,               \
                                   &_class##_ATX_ReferenceableInterface,        \
                                   sizeof(_class),                              \
                                   object);                                     \
}

/** @} */

#endif /* _BLT_MODULE_H_ */




