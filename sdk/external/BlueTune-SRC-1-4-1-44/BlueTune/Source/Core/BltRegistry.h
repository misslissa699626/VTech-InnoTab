/*****************************************************************
|
|   BlueTune - Registry API
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BLT_Registry interface
 */

#ifndef _BLT_REGISTRY_H_
#define _BLT_REGISTRY_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltDefs.h"
#include "BltTypes.h"
#include "BltErrors.h"
#include "BltMedia.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct BLT_RegistryKey BLT_RegistryKey;

typedef enum {
    BLT_REGISTRY_VALUE_TYPE_NONE,
    BLT_REGISTRY_VALUE_TYPE_STRING,
    BLT_REGISTRY_VALUE_TYPE_INTEGER,
    BLT_REGISTRY_VALUE_TYPE_BOOLEAN,
    BLT_REGISTRY_VALUE_TYPE_RAW_DATA
} BLT_RegistryValueType;

typedef struct {
    BLT_Size size;
    BLT_Any  buffer;
} BLT_RegistryRawData;

typedef union {
    BLT_CString         string;
    BLT_Boolean         boolean;
    BLT_UInt32          integer;
    BLT_RegistryRawData data;
} BLT_RegistryValue;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_REGISTRY_KEY_ROOT                       ((BLT_RegistryKey*)0)
#define BLT_REGISTRY_KEY_PUBLIC                     "BLT_PUBLIC"
#define BLT_REGISTRY_KEY_PRIVATE                    "BLT_PRIVATE"
#define BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS   "MediaTypes"

#define BLT_ERROR_NO_SUCH_KEY             (BLT_ERROR_BASE_REGISTRY - 0)
#define BLT_ERROR_KEY_VALUE_TYPE_MISMATCH (BLT_ERROR_BASE_REGISTRY - 1)

/*----------------------------------------------------------------------
|   BLT_Registry Interface
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_Registry)
/**
 * @brief Interface implemented by the registry of the BlueTune system
 *  
 */
ATX_BEGIN_INTERFACE_DEFINITION(BLT_Registry)
    BLT_Result (*CreateKey)(BLT_Registry*     self,
                            BLT_RegistryKey*  parent,
                            BLT_CString       name,
                            BLT_RegistryKey** key);

    BLT_Result (*DestroyKey)(BLT_Registry*    self,
                             BLT_RegistryKey* key);

    BLT_Result (*GetKey)(BLT_Registry*     self,
                         BLT_RegistryKey*  parent,
                         BLT_CString       name,
                         BLT_RegistryKey** key);

    BLT_Result (*SetKeyValue)(BLT_Registry*         self,
                              BLT_RegistryKey*      parent,
                              BLT_CString           name,
                              BLT_RegistryValueType value_type,
                              BLT_RegistryValue*    value);

    BLT_Result (*GetKeyValue)(BLT_Registry*          self,
                              BLT_RegistryKey*       parent,
                              BLT_CString            name,
                              BLT_RegistryValueType* value_type,
                              BLT_RegistryValue*     value);
    BLT_Result (*RegisterName)(BLT_Registry*         self, 
                               BLT_CString           category,
                               BLT_CString           name,
                               BLT_UInt32*           id);
    BLT_Result (*RegisterNameForId)(BLT_Registry* self, 
                                    BLT_CString   category,
                                    BLT_CString   name,
                                    BLT_UInt32    id);
    BLT_Result (*GetNameForId)(BLT_Registry*         self, 
                               BLT_CString           category,
                               BLT_UInt32            id,
                               BLT_CString*          name);
    BLT_Result (*GetIdForName)(BLT_Registry*         self, 
                               BLT_CString           category,
                               BLT_CString           name,
                               BLT_UInt32*           id);
    BLT_Result (*RegisterExtension)(BLT_Registry*         self,
                                    BLT_CString           extension,
                                    BLT_CString           media_type);
    BLT_Result (*GetMediaTypeIdForExtension)(BLT_Registry*    self,
                                             BLT_CString      extension,
                                             BLT_MediaTypeId* type_id);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define BLT_Registry_CreateKey(object, parent, name, key) \
ATX_INTERFACE(object)->CreateKey(object, parent, name, key)

#define BLT_Registry_DestroyKey(object, key) \
ATX_INTERFACE(object)->DestroyKey(object, key)

#define BLT_Registry_GetKey(object, parent, name, key) \
ATX_INTERFACE(object)->GetKey(object, parent, name, key)

#define BLT_Registry_SetKeyValue(object, parent, name, value_type, value) \
ATX_INTERFACE(object)->SetKeyValue(object, \
parent, name, value_type, value)

#define BLT_Registry_GetKeyValue(object, parent, name, value_type, value) \
ATX_INTERFACE(object)->GetKeyValue(object, \
parent, name, value_type, value)

#define BLT_Registry_RegisterName(object, category, name, id) \
ATX_INTERFACE(object)->RegisterName(object, category, \
name, id)

#define BLT_Registry_RegisterNameForId(object, category, name, id) \
ATX_INTERFACE(object)->RegisterNameForId(object, category, \
name, id)

#define BLT_Registry_GetNameForId(object, category, name, id) \
ATX_INTERFACE(object)->GetNameForId(object, category, name, id)

#define BLT_Registry_GetIdForName(object, category, name, id) \
ATX_INTERFACE(object)->GetIdForName(object, category, \
name, id)

#define BLT_Registry_RegisterExtension(object, extension, media_type) \
ATX_INTERFACE(object)->RegisterExtension(object, \
extension, media_type)

#define BLT_Registry_GetMediaTypeIdForExtension(object, extension, type_id) \
ATX_INTERFACE(object)->GetMediaTypeIdForExtension(object, \
extension, type_id)

#define BLT_Registry_Destroy(object) ATX_DESTROY_OBJECT(object)

#endif /* _BLT_REGISTRY_H_ */
