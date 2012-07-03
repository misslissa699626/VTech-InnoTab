/*****************************************************************
|
|   BlueTune - Registry Object
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BlueTune Registry
 */

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltTypes.h"
#include "BltDefs.h"
#include "BltErrors.h"
#include "BltRegistry.h"
#include "BltRegistryPriv.h"
#include "BltStreamPriv.h"
#include "BltMediaNode.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
/*ATX_SET_LOCAL_LOGGER("bluetune.core.registry")*/

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef BLT_RegistryKey Key;

struct BLT_RegistryKey {
    char*                 name;
    BLT_RegistryValueType value_type;
    BLT_RegistryValue     value;
    BLT_RegistryKey*      prev;
    BLT_RegistryKey*      next;
    BLT_RegistryKey*      parent;
    BLT_RegistryKey*      children;
    unsigned long         hash;
};

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_Registry);
    ATX_IMPLEMENTS(ATX_Destroyable);
    
    /* members */
    Key*       root;
    BLT_UInt32 id_base;
} Registry;

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define BLT_REGISTRY_NAME_TO_ID_NAMESPACE       "NameToIdMaps"
#define BLT_REGISTRY_ID_TO_NAME_NAMESPACE       "IdToNameMaps"
#define BLT_REGISTRY_FILE_EXTENSIONS_NAMESPACE  "FileExtensions"

#define BLT_REGISTRY_MEDIA_TYPE_SUBKEY          "MediaType"

/*----------------------------------------------------------------------
|    forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(Registry, BLT_Registry)
ATX_DECLARE_INTERFACE_MAP(Registry, ATX_Destroyable)
static BLT_Result Registry_RegisterNameAndId(Registry*   registry, 
                                             BLT_CString category, 
                                             BLT_CString name, 
                                             BLT_UInt32  id,
                                             BLT_Boolean register_reverse_mapping);
BLT_METHOD Registry_CreateKey(BLT_Registry*     self,
                              BLT_RegistryKey*  parent,
                              BLT_CString       name,
                              BLT_RegistryKey** key);
BLT_METHOD Registry_GetIdForName(BLT_Registry* instance, 
                                 BLT_CString   category,
                                 BLT_CString   name,
                                 BLT_UInt32*  id);

/*----------------------------------------------------------------------
|    ComputeHash
+---------------------------------------------------------------------*/
static unsigned long
ComputeHash(BLT_CString string, BLT_Size size)
{
    unsigned long hash = *string << 7;

    while (size--) {
        hash = (1000003*hash) ^ *string++;
    }
    return hash ^ size;
}

/*----------------------------------------------------------------------
|    Key_DestroyValue
+---------------------------------------------------------------------*/
static void
Key_DestroyValue(Key* key)
{
    /* free the memory for the value */
    switch (key->value_type) {
      case BLT_REGISTRY_VALUE_TYPE_STRING:
        if (key->value.string) {
            ATX_FreeMemory((void*)key->value.string);
        }
        break;

      case BLT_REGISTRY_VALUE_TYPE_RAW_DATA:
        if (key->value.data.buffer) {
            ATX_FreeMemory(key->value.data.buffer);
        }
        break;

      default:
        break;
    }
}

/*----------------------------------------------------------------------
|    Key_Create
+---------------------------------------------------------------------*/
static Key*
Key_Create(BLT_CString name, BLT_Size size)
{
    Key* key;

    /* allocate a key */
    key = ATX_AllocateZeroMemory(sizeof(Key));
    if (key == NULL) return NULL;

    /* copy the name */
    key->name = (char*)ATX_AllocateMemory(size+1);
    if (key->name == NULL) {
        ATX_FreeMemory(key);
        return NULL;
    }
    key->name[size] = '\0';
    ATX_CopyMemory(key->name, name, size);
    key->hash = ComputeHash(name, size);

    return key;
}

/*----------------------------------------------------------------------
|    Key_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
Key_Destroy(Key* key)
{
    /* free the name string */
    if (key->name) {
        ATX_FreeMemory((void*)key->name);
    }
        
    /* destroy the key value */
    Key_DestroyValue(key);
        
    /* destroy the subkeys */
    {       
        Key* subkey = key->children;
        key->children = NULL;
        while (subkey) {
            Key* next = subkey->next;
            subkey->next = subkey->prev = NULL;
            Key_Destroy(subkey);
            subkey = next;
        }
    }

    /* relink neighbors and parent */
    if (key->next) {
        key->next->prev = key->prev;
    }
    if (key->prev) {
        key->prev->next = key->next;

        /* if we're the first child of the parent, update the parent */
        if (key->parent == key) {
            key->parent->children = key->next;
        }
    }

    /* destroy the key */
    ATX_FreeMemory((void*)key);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Key_FindSubKey
+---------------------------------------------------------------------*/
static Key* 
Key_FindSubKey(Key* key, BLT_CString name, BLT_Size size)
{
    unsigned long hash = ComputeHash(name, size);
    Key*          subkey = key->children;
    
    /* find a key within the subkeys */
    while (subkey) {
        if (subkey->hash == hash) {
            /* possible match */
            if (ATX_StringsEqualN(subkey->name, name, size)) {
                return subkey;
            }
        }
        subkey = subkey->next;
    }

    return NULL;
}

/*----------------------------------------------------------------------
|    Key_AddSubKey
+---------------------------------------------------------------------*/
static Key* 
Key_AddSubKey(Key* key, BLT_CString name, BLT_Size size)
{
    Key* new_key;
    
    /* create a new key */
    new_key = Key_Create(name, size);
    if (new_key == NULL) return NULL;

    /* link the new key */
    new_key->next = key->children;
    if (new_key->next) {
        new_key->next->prev = new_key;
    }
    key->children = new_key;

    return new_key;
}

/*----------------------------------------------------------------------
|    Registry_Initialize
+---------------------------------------------------------------------*/
static BLT_Result
Registry_Initialize(Registry* registry)
{
    BLT_Result result;

    /* initial id base */
    registry->id_base = 100;

    /* create the root key */
    registry->root = Key_Create("@", 1);
    if (registry->root == NULL) {
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* create the media type namespace key */
    result = Registry_CreateKey(&ATX_BASE(registry, BLT_Registry),
                                BLT_REGISTRY_KEY_ROOT,
                                BLT_REGISTRY_KEY_PUBLIC "/"
                                BLT_REGISTRY_ID_TO_NAME_NAMESPACE "/"
                                BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                NULL);
    if (BLT_FAILED(result)) return result;
    result = Registry_CreateKey(&ATX_BASE(registry, BLT_Registry), 
                                BLT_REGISTRY_KEY_ROOT,
                                BLT_REGISTRY_KEY_PUBLIC "/"
                                BLT_REGISTRY_NAME_TO_ID_NAMESPACE "/"
                                BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                NULL);
    if (BLT_FAILED(result)) return result;

    /* register the default types and formats */
    Registry_RegisterNameAndId(registry, 
                               BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                               "none", BLT_MEDIA_TYPE_ID_NONE, BLT_TRUE);
    Registry_RegisterNameAndId(registry, 
                               BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                               "unknown", BLT_MEDIA_TYPE_ID_UNKNOWN, BLT_TRUE);
    Registry_RegisterNameAndId(registry, 
                               BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                               "audio", BLT_MEDIA_TYPE_ID_AUDIO, BLT_TRUE);
    Registry_RegisterNameAndId(registry, 
                               BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                               "audio/pcm", BLT_MEDIA_TYPE_ID_AUDIO_PCM, BLT_TRUE);
    Registry_RegisterNameAndId(registry, 
                               BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                               "video", BLT_MEDIA_TYPE_ID_VIDEO, BLT_TRUE);
    Registry_RegisterNameAndId(registry, 
                               BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                               "video/raw", BLT_MEDIA_TYPE_ID_VIDEO_RAW, BLT_TRUE);

    /* create the file extensions namespace key */
    result = Registry_CreateKey(&ATX_BASE(registry, BLT_Registry), 
                                BLT_REGISTRY_KEY_ROOT,
                                BLT_REGISTRY_KEY_PUBLIC "/"
                                BLT_REGISTRY_FILE_EXTENSIONS_NAMESPACE,
                                NULL);
    if (BLT_FAILED(result)) return result;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Registry_Create
+---------------------------------------------------------------------*/
BLT_Result
Registry_Create(BLT_Registry** object)
{
    Registry*  registry;
    BLT_Result result;

    /* allocate memory for the object */
    registry = ATX_AllocateZeroMemory(sizeof(Registry));
    if (registry == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* initialize the registry */
    result = Registry_Initialize(registry);
    if (BLT_FAILED(result)) {
        ATX_FreeMemory(registry);
        object = NULL;
        return result;
    }

    /* setup interfaces */
    ATX_SET_INTERFACE(registry, Registry, BLT_Registry);
    ATX_SET_INTERFACE(registry, Registry, ATX_Destroyable);
    *object = &ATX_BASE(registry, BLT_Registry);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Registry_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
Registry_Destroy(ATX_Destroyable* _self)
{
    Registry* self = ATX_SELF(Registry, ATX_Destroyable);

    /* destroy to root key table */
    Key_Destroy(self->root);

    /* free the object memory */
    ATX_FreeMemory((void*)self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Registry_FindKeyByName
+---------------------------------------------------------------------*/
static Key*
Registry_FindKeyByName(Registry*   registry, 
                       Key*        parent, 
                       BLT_CString name,
                       BLT_Boolean create)
{
    const char* look = name;
    Key*        key = parent ? parent : registry->root;

    /* a NULL or empty name is equivalent to the parent */
    if (name == NULL || *name == '\0') return parent;

    /* parse the name and descend */
    do {
        if (*look == '/' || *look == '\0') {
            /* find the first part of the key in the subkeys */
            if (look != name) {
                Key* subkey = Key_FindSubKey(key, name, (BLT_Size)(look-name));
                if (subkey == NULL) {
                    /* not found */
                    if (create) {
                        /* create the key */
                        subkey = Key_AddSubKey(key, name, (BLT_Size)(look-name));
                    } else {
                        /* we're done */
                        return NULL;
                    }
                }
                key = subkey;
            }

            /* move to the next part of the name */
            name = look+1;
        }
    } while (*look++);

    return key;
}

/*----------------------------------------------------------------------
|    Registry_CreateKey
+---------------------------------------------------------------------*/
BLT_METHOD 
Registry_CreateKey(BLT_Registry*     _self,
                   BLT_RegistryKey*  parent,
                   BLT_CString       name,
                   BLT_RegistryKey** key)
{
    Registry* self = ATX_SELF(Registry, BLT_Registry);
    Key*      new_key;

    new_key = Registry_FindKeyByName(self, parent, name, BLT_TRUE);
    if (key) *key = new_key;
    if (new_key == NULL) {
        return BLT_FAILURE;
    } else {
        return BLT_SUCCESS;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Registry_DestroyKey
+---------------------------------------------------------------------*/
BLT_METHOD 
Registry_DestroyKey(BLT_Registry* self, BLT_RegistryKey* key)
{
    BLT_COMPILER_UNUSED(self);
    return Key_Destroy(key);
}

/*----------------------------------------------------------------------
|    Registry_GetKey
+---------------------------------------------------------------------*/
BLT_METHOD
Registry_GetKey(BLT_Registry*     _self,
                BLT_RegistryKey*  parent,
                BLT_CString       name,
                BLT_RegistryKey** key)
{
    Registry* self = ATX_SELF(Registry, BLT_Registry);

    *key = Registry_FindKeyByName(self, parent, name, BLT_FALSE);
    if (*key) {
        return BLT_SUCCESS;
    } else {
        return BLT_ERROR_NO_SUCH_KEY;
    }
}

/*----------------------------------------------------------------------
|    Registry_SetKeyValue
+---------------------------------------------------------------------*/
BLT_METHOD
Registry_SetKeyValue(BLT_Registry*         _self,
                     BLT_RegistryKey*      parent,
                     BLT_CString           name,
                     BLT_RegistryValueType value_type,
                     BLT_RegistryValue*    value)
{
    Registry* self = ATX_SELF(Registry, BLT_Registry);
    Key*      key;

    /* get the key */
    key = Registry_FindKeyByName(self, parent, name, BLT_TRUE);
    if (key == NULL) return BLT_FAILURE;
    
    /* reset the value of the key */
    Key_DestroyValue(key);
    
    /* set the new value of the key */
    key->value_type = value_type;
    switch (value_type) {
      case BLT_REGISTRY_VALUE_TYPE_STRING:
        key->value.string = ATX_DuplicateString(value->string);
        break;

      case BLT_REGISTRY_VALUE_TYPE_INTEGER:
        key->value.integer = value->integer;
        break;

      case BLT_REGISTRY_VALUE_TYPE_BOOLEAN:
        key->value.boolean = value->boolean;
        break;

      case BLT_REGISTRY_VALUE_TYPE_RAW_DATA:
        key->value.data.size = value->data.size;
        key->value.data.buffer = ATX_AllocateMemory(value->data.size);
        if (key->value.data.buffer == NULL) {
            key->value.data.size = 0;
            return BLT_ERROR_OUT_OF_MEMORY;
        }
        ATX_CopyMemory(key->value.data.buffer, 
                       value->data.buffer, 
                       value->data.size);
        break;

      case BLT_REGISTRY_VALUE_TYPE_NONE:
        break;
    }

    /*Key_Dump(registry->root, 0);*/

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Registry_GetKeyValue
+---------------------------------------------------------------------*/
BLT_METHOD
Registry_GetKeyValue(BLT_Registry*          _self,
                     BLT_RegistryKey*       parent,
                     BLT_CString            name,
                     BLT_RegistryValueType* value_type,
                     BLT_RegistryValue*     value)
{
    Registry* self = ATX_SELF(Registry, BLT_Registry);
    Key*      key;

    /* get the key */
    key = Registry_FindKeyByName(self, parent, name, BLT_FALSE);
    if (key == NULL) return BLT_FAILURE;
    
    /* return the value */
    if (value_type) {
        *value_type = key->value_type;
    }
    if (value) {
        *value = key->value;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    IdToName
+---------------------------------------------------------------------*/
static void
IdToName(unsigned long id, char* name)
{
    unsigned nibble;
    int      i;

    for (i=0; i<8; i++) {
        nibble = id & 0x0F;
        name[7-i] = nibble >= 10 ? 'A' + (nibble-10) : '0' + nibble;
        id >>= 4;
    }
}

/*----------------------------------------------------------------------
|    Registry_RegisterNameAndId
+---------------------------------------------------------------------*/
static BLT_Result
Registry_RegisterNameAndId(Registry*   registry, 
                           BLT_CString category, 
                           BLT_CString name,
                           BLT_UInt32  id,
                           BLT_Boolean register_reverse_mapping)
{
    BLT_RegistryKey* namespace_key;
    BLT_RegistryKey* category_key;
    BLT_Result       result;

    /* get the key for the namespace */
    result = Registry_GetKey(&ATX_BASE(registry, BLT_Registry), 
                             BLT_REGISTRY_KEY_ROOT,
                             BLT_REGISTRY_KEY_PUBLIC "/"
                             BLT_REGISTRY_NAME_TO_ID_NAMESPACE,
                             &namespace_key);
    if (BLT_FAILED(result)) return result;

    /* get the key for the category */
    result = Registry_GetKey(&ATX_BASE(registry, BLT_Registry), 
                             namespace_key, 
                             category, 
                             &category_key);
    if (BLT_FAILED(result)) return result;
        
    /* set the value for the key */
    result = Registry_SetKeyValue(&ATX_BASE(registry, BLT_Registry),
                                  category_key, 
                                  name,
                                  BLT_REGISTRY_VALUE_TYPE_INTEGER, 
                                  (BLT_RegistryValue*)&id);
    if (BLT_FAILED(result)) return result;

    /* stop now if we dont' want to create a reverse mapping */
    if (!register_reverse_mapping) return BLT_SUCCESS;
    
    /* create the key for the reverse mapping */
    result = Registry_GetKey(&ATX_BASE(registry, BLT_Registry), 
                             BLT_REGISTRY_KEY_ROOT,
                             BLT_REGISTRY_KEY_PUBLIC "/"
                             BLT_REGISTRY_ID_TO_NAME_NAMESPACE,
                             &namespace_key);
    if (BLT_FAILED(result)) return result;
    result = Registry_GetKey(&ATX_BASE(registry, BLT_Registry), 
                             namespace_key, 
                             category, 
                             &category_key);
    if (BLT_FAILED(result)) return result;
    {
        char id_name[8+1];
        id_name[8] = '\0';
        IdToName(id, id_name);
        result = Registry_SetKeyValue(&ATX_BASE(registry, BLT_Registry), 
                                      category_key, 
                                      id_name,
                                      BLT_REGISTRY_VALUE_TYPE_STRING, 
                                      (BLT_RegistryValue*)&name);
        if (BLT_FAILED(result)) return result;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Registry_RegisterName
+---------------------------------------------------------------------*/
BLT_METHOD
Registry_RegisterName(BLT_Registry* _self, 
                      BLT_CString   category,
                      BLT_CString   name,
                      BLT_UInt32*   id)
{
    Registry*  self = ATX_SELF(Registry, BLT_Registry);
    BLT_Result result;

    /* check if the name has already been registered */
    result = Registry_GetIdForName(_self, category, name, id);
    if (BLT_SUCCEEDED(result)) return BLT_SUCCESS;

    /* the name is not registered, register it now */
    if (id) {
        *id = self->id_base;
    }
    return Registry_RegisterNameAndId(self, 
                                      category, 
                                      name, 
                                      self->id_base++,
                                      BLT_TRUE);
}

/*----------------------------------------------------------------------
|    Registry_RegisterNameForId
+---------------------------------------------------------------------*/
BLT_METHOD
Registry_RegisterNameForId(BLT_Registry* _self, 
                           BLT_CString   category,
                           BLT_CString   name,
                           BLT_UInt32    id)
{
    Registry*  self = ATX_SELF(Registry, BLT_Registry);
    BLT_UInt32 existing_id;
    BLT_Result result;

    /* check if the name has already been registered */
    result = Registry_GetIdForName(_self, category, name, &existing_id);
    if (BLT_SUCCEEDED(result)) {
        return existing_id == id ? BLT_SUCCESS : BLT_ERROR_INVALID_PARAMETERS;
    }
    
    /* register the name, but without a reverse mapping */
    return Registry_RegisterNameAndId(self, 
                                      category, 
                                      name, 
                                      id, 
                                      BLT_FALSE);
}

/*----------------------------------------------------------------------
|    Registry_GetNameForId
+---------------------------------------------------------------------*/
BLT_METHOD
Registry_GetNameForId(BLT_Registry* self, 
                      BLT_CString   category,
                      BLT_UInt32    id,
                      BLT_CString*  name)
{
    BLT_RegistryKey*      category_key;
    BLT_RegistryKey*      namespace_key;
    BLT_RegistryValue     key_value;
    BLT_RegistryValueType key_value_type;
    char                  id_name[8+1];
    BLT_Result            result;

    /* convert the id into a name */
    id_name[8] = '\0';
    IdToName(id, id_name);

    /* get the key for the namespace */
    result = Registry_GetKey(self,
                             BLT_REGISTRY_KEY_ROOT,
                             BLT_REGISTRY_KEY_PUBLIC "/"
                             BLT_REGISTRY_ID_TO_NAME_NAMESPACE,
                             &namespace_key);
    if (BLT_FAILED(result)) return result;

    /* get the key for the category */
    result = Registry_GetKey(self,
                             namespace_key, 
                             category, 
                             &category_key);
    if (BLT_FAILED(result)) return result;

    /* get the key value */
    result = Registry_GetKeyValue(self, 
                                  category_key, 
                                  id_name, 
                                  &key_value_type,
                                  &key_value);
    if (BLT_FAILED(result)) return result;

    /* return the name */
    if (key_value_type != BLT_REGISTRY_VALUE_TYPE_STRING) {
        return BLT_ERROR_KEY_VALUE_TYPE_MISMATCH;
    }
    if (name) {
        *name = key_value.string;
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Registry_GetIdForName
+---------------------------------------------------------------------*/
BLT_METHOD
Registry_GetIdForName(BLT_Registry* self, 
                      BLT_CString   category,
                      BLT_CString   name,
                      BLT_UInt32*   id)
{
    BLT_RegistryKey*      category_key;
    BLT_RegistryKey*      namespace_key;
    BLT_RegistryValue     key_value;
    BLT_RegistryValueType key_value_type;
    BLT_Result            result;

    /* get the key for the namespace */
    result = Registry_GetKey(self,
                             BLT_REGISTRY_KEY_ROOT,
                             BLT_REGISTRY_KEY_PUBLIC "/"
                             BLT_REGISTRY_NAME_TO_ID_NAMESPACE,
                             &namespace_key);
    if (BLT_FAILED(result)) return result;

    /* get the key for the category */
    result = Registry_GetKey(self,
                             namespace_key, 
                             category, 
                             &category_key);
    if (BLT_FAILED(result)) return result;

    /* get the key value */
    result = Registry_GetKeyValue(self, 
                                  category_key, 
                                  name, 
                                  &key_value_type,
                                  &key_value);
    if (BLT_FAILED(result)) return result;

    /* return the name */
    if (key_value_type != BLT_REGISTRY_VALUE_TYPE_INTEGER) {
        return BLT_ERROR_KEY_VALUE_TYPE_MISMATCH;
    }
    if (id) {
        *id = key_value.integer;
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Registry_RegisterExtension
+---------------------------------------------------------------------*/
BLT_METHOD
Registry_RegisterExtension(BLT_Registry* self, 
                           BLT_CString   extension,
                           BLT_CString   media_type)
{
    BLT_RegistryKey*  namespace_key;
    BLT_RegistryKey*  extension_key;
    BLT_UInt32        type_id;
    BLT_RegistryValue value;
    BLT_Result        result;

    /* make sure that the media type is registered */
    result = Registry_RegisterName(self, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   media_type,
                                   &type_id);
    if (BLT_FAILED(result)) return result;

    /* get the namespace key */
    result = Registry_GetKey(self, 
                             BLT_REGISTRY_KEY_ROOT,
                             BLT_REGISTRY_KEY_PUBLIC "/"
                             BLT_REGISTRY_FILE_EXTENSIONS_NAMESPACE,
                             &namespace_key);
    if (BLT_FAILED(result)) return result;

    /* create/get the key for the file extension */
    result = Registry_CreateKey(self,
                                namespace_key,
                                extension,
                                &extension_key);
    if (BLT_FAILED(result)) return result;

    /* set the value for the media type subkey */
    value.integer = type_id;
    result =  Registry_SetKeyValue(self, 
                                   extension_key, 
                                   BLT_REGISTRY_MEDIA_TYPE_SUBKEY,
                                   BLT_REGISTRY_VALUE_TYPE_INTEGER, 
                                   &value);
    if (BLT_FAILED(result)) return result;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Registry_GetMediaTypeIdForExtension
+---------------------------------------------------------------------*/
BLT_METHOD
Registry_GetMediaTypeIdForExtension(BLT_Registry*    self, 
                                    BLT_CString      extension,
                                    BLT_MediaTypeId* type_id)
{
    BLT_RegistryKey*      namespace_key;
    BLT_RegistryKey*      extension_key;
    BLT_RegistryValue     value;
    BLT_RegistryValueType value_type;
    BLT_Result            result;

    /* clear the type (in case of early return) */
    *type_id = BLT_MEDIA_TYPE_ID_NONE;

    /* get the namespace key */
    result = Registry_GetKey(self, 
                             BLT_REGISTRY_KEY_ROOT,
                             BLT_REGISTRY_KEY_PUBLIC "/"
                             BLT_REGISTRY_FILE_EXTENSIONS_NAMESPACE,
                             &namespace_key);
    if (BLT_FAILED(result)) return result;

    /* get the key for the file extension */
    {
        ATX_String extension_lc = ATX_String_Create(extension);
        ATX_String_MakeLowercase(&extension_lc);
        result = Registry_GetKey(self, 
                                 namespace_key, 
                                 ATX_CSTR(extension_lc), 
                                 &extension_key);
        ATX_String_Destruct(&extension_lc);
        if (BLT_FAILED(result)) return result;
    }

    /* get the media type id */
    result = Registry_GetKeyValue(self, extension_key,
                                  BLT_REGISTRY_MEDIA_TYPE_SUBKEY,
                                  &value_type,
                                  &value);
    if (BLT_FAILED(result)) return result;
    if (value_type != BLT_REGISTRY_VALUE_TYPE_INTEGER) {
        return BLT_ERROR_KEY_VALUE_TYPE_MISMATCH;
    }
    
    /* return the values */
    *type_id = value.integer;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(Registry)
    ATX_GET_INTERFACE_ACCEPT(Registry, BLT_Registry)
    ATX_GET_INTERFACE_ACCEPT(Registry, ATX_Destroyable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_Registry interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(Registry, BLT_Registry)
    Registry_CreateKey,
    Registry_DestroyKey,
    Registry_GetKey,
    Registry_SetKeyValue,
    Registry_GetKeyValue,
    Registry_RegisterName,
    Registry_RegisterNameForId,
    Registry_GetNameForId,
    Registry_GetIdForName,
    Registry_RegisterExtension,
    Registry_GetMediaTypeIdForExtension
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_Destroyable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_DESTROYABLE_INTERFACE(Registry)

