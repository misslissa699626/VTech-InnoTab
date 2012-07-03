/*****************************************************************
|
|   Atomix - Properties Interface
|
| Copyright (c) 2002-2010, Axiomatic Systems, LLC.
| All rights reserved.
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions are met:
|     * Redistributions of source code must retain the above copyright
|       notice, this list of conditions and the following disclaimer.
|     * Redistributions in binary form must reproduce the above copyright
|       notice, this list of conditions and the following disclaimer in the
|       documentation and/or other materials provided with the distribution.
|     * Neither the name of Axiomatic Systems nor the
|       names of its contributors may be used to endorse or promote products
|       derived from this software without specific prior written permission.
|
| THIS SOFTWARE IS PROVIDED BY AXIOMATIC SYSTEMS ''AS IS'' AND ANY
| EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
| WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
| DISCLAIMED. IN NO EVENT SHALL AXIOMATIC SYSTEMS BE LIABLE FOR ANY
| DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
| (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
| ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
| SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "AtxInterfaces.h"
#include "AtxTypes.h"
#include "AtxUtils.h"
#include "AtxResults.h"
#include "AtxDestroyable.h"
#include "AtxProperties.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct PropertyNode {
    ATX_Property         property;
    struct PropertyNode* next;
} PropertyNode;

typedef struct PropertyListenerNode {
    ATX_CString                  name;
    ATX_PropertyListener*        listener;
    struct PropertyListenerNode* next;
} PropertyListenerNode;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(ATX_Properties);
    ATX_IMPLEMENTS(ATX_Destroyable);

    /* members */
    PropertyNode*         property_nodes;
    PropertyListenerNode* listener_nodes;
} Properties;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(ATX_Iterator);
    ATX_IMPLEMENTS(ATX_Destroyable);

    /* members */
    Properties*   properties;
    PropertyNode* node;
} PropertiesIterator;

/*----------------------------------------------------------------------
|  forward declarations
+---------------------------------------------------------------------*/
ATX_INTERFACE_MAP(Properties, ATX_Properties);
ATX_INTERFACE_MAP(Properties, ATX_Destroyable);
ATX_INTERFACE_MAP(PropertiesIterator, ATX_Iterator);
ATX_INTERFACE_MAP(PropertiesIterator, ATX_Destroyable);

/*----------------------------------------------------------------------
|   PropertiesIterator_Create
+---------------------------------------------------------------------*/
static ATX_Result
PropertiesIterator_Create(Properties*    properties,
                          ATX_Iterator** object)
{
    PropertiesIterator* iterator;

    /* create a new object */
    iterator = (PropertiesIterator*)
        ATX_AllocateMemory(sizeof(PropertiesIterator));
    if (iterator == NULL) {
        *object = NULL;
        return ATX_ERROR_OUT_OF_MEMORY;
    }
    
    /* construct the object */
    iterator->properties = properties;
    iterator->node = properties->property_nodes;

    /* setup the interfaces */
    ATX_SET_INTERFACE(iterator, PropertiesIterator, ATX_Iterator);
    ATX_SET_INTERFACE(iterator, PropertiesIterator, ATX_Destroyable);
    *object = &ATX_BASE(iterator, ATX_Iterator);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   PropertiesIterator_Destroy
+---------------------------------------------------------------------*/
ATX_METHOD
PropertiesIterator_Destroy(ATX_Destroyable* _self)
{
    ATX_FreeMemory((void*)ATX_SELF(Properties, ATX_Destroyable));

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   PropertiesIterator_GetNext
+---------------------------------------------------------------------*/
ATX_METHOD
PropertiesIterator_GetNext(ATX_Iterator* _self,
                           ATX_Any*      item)
{
    PropertiesIterator* self = ATX_SELF(PropertiesIterator, ATX_Iterator);
    if (self->node) {
        *item = (ATX_Any)self->node;
        self->node = self->node->next;
    } else {
        *item = (ATX_Any)NULL;
        return ATX_ERROR_END_OF_ITERATION;
    }
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   PropertiesIterator_Reset
+---------------------------------------------------------------------*/
ATX_METHOD
PropertiesIterator_Reset(ATX_Iterator* _self)
{
    PropertiesIterator* self = ATX_SELF(PropertiesIterator, ATX_Iterator);

    /* reset to the first node */
    self->node = self->properties->property_nodes;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(PropertiesIterator) 
    ATX_GET_INTERFACE_ACCEPT(PropertiesIterator, ATX_Iterator)
    ATX_GET_INTERFACE_ACCEPT(PropertiesIterator, ATX_Destroyable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   ATX_Destroyable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_DESTROYABLE_INTERFACE(PropertiesIterator)

/*----------------------------------------------------------------------
|   ATX_Iterator interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(PropertiesIterator, ATX_Iterator)
    PropertiesIterator_GetNext,
    PropertiesIterator_Reset
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   PropertyNode_SetValue
+---------------------------------------------------------------------*/
static ATX_Result
PropertyNode_SetValue(PropertyNode*            node,
                      const ATX_PropertyValue* value)
{
    return ATX_PropertyValue_Clone(value, &node->property.value);
}

/*----------------------------------------------------------------------
|   PropertyNode_Create
+---------------------------------------------------------------------*/
static PropertyNode*
PropertyNode_Create(ATX_CString              name, 
                    const ATX_PropertyValue* value)
{
    PropertyNode* node;

    /* allocate a new node */
    node = (PropertyNode*)ATX_AllocateMemory(sizeof(PropertyNode));
    if (node == NULL) return NULL;

    /* construct the node */
    node->next = NULL;
    node->property.name = ATX_DuplicateString(name);
    if (ATX_FAILED(PropertyNode_SetValue(node, value))) {
        ATX_FreeMemory((void*)node->property.name);
        ATX_FreeMemory((void*)node);
        return NULL;
    }

    return node;
}

/*----------------------------------------------------------------------
|   PropertyNode_Destroy
+---------------------------------------------------------------------*/
static void
PropertyNode_Destroy(PropertyNode* node)
{
    /* destruct the node property */
    ATX_Property_Destruct(&node->property);

    /* free the node */
    ATX_FreeMemory((void*)node);
}

/*----------------------------------------------------------------------
|   PropertyListenerNode_Create
+---------------------------------------------------------------------*/
static PropertyListenerNode*
PropertyListenerNode_Create(ATX_CString           name, 
                            ATX_PropertyListener* listener)
{
    PropertyListenerNode* node;

    /* allocate a new node */
    node = (PropertyListenerNode*)
        ATX_AllocateMemory(sizeof(PropertyListenerNode));
    if (node == NULL) return NULL;

    /* construct the node */
    node->next     = NULL;
    node->listener = listener;
    if (name != NULL) {
        node->name = ATX_DuplicateString(name);
    } else {
        node->name = NULL;
    }

    return node;
}

/*----------------------------------------------------------------------
|   PropertyListenerNode_Destroy
+---------------------------------------------------------------------*/
static void
PropertyListenerNode_Destroy(PropertyListenerNode* node)
{
    /* destruct the node */
    if (node->name != NULL) {
        ATX_FreeMemory((void*)node->name);
    }

    /* free the node */
    ATX_FreeMemory((void*)node);
}

/*----------------------------------------------------------------------
|   Properties_NotifyListeners
+---------------------------------------------------------------------*/
static void
Properties_NotifyListeners(Properties*              self,
                           ATX_CString              name,
                           const ATX_PropertyValue* value)
{
    PropertyListenerNode* node;

    /* look for listeners registered for this name */
    node = self->listener_nodes;
    while (node) {
        if (node->name == NULL || ATX_StringsEqual(node->name, name)) {
            ATX_PropertyListener_OnPropertyChanged(node->listener,
                                                   name,
                                                   value);
        }
        node = node->next;
    }
}

/*----------------------------------------------------------------------
|   Properties_FindProperty
+---------------------------------------------------------------------*/
static PropertyNode*
Properties_FindProperty(Properties* self, ATX_CString name)
{
    PropertyNode* node; 

    /* find the node with that name */
    node = self->property_nodes;
    while (node) {
        if (ATX_StringsEqual(node->property.name, name)) {
            /* match */
            return node;
        }
        node = node->next;
    }

    /* no match */
    return NULL;
}

/*----------------------------------------------------------------------
|   Properties_GetProperty
+---------------------------------------------------------------------*/
ATX_METHOD
Properties_GetProperty(ATX_Properties*    _self,
                       ATX_CString        name,
                       ATX_PropertyValue* value)
{
    Properties*   self = ATX_SELF(Properties, ATX_Properties);
    PropertyNode* node; 

    /* check parameters */
    if (self == NULL || name == NULL) {
        return ATX_ERROR_INVALID_PARAMETERS;
    }
    
    /* find the node with that name */
    node = Properties_FindProperty(self, name);
    if (node) {
        *value = node->property.value;
        return ATX_SUCCESS;
    } else {
        return ATX_ERROR_NO_SUCH_PROPERTY;
    }
}

/*----------------------------------------------------------------------
|   Properties_DeleteProperty
+---------------------------------------------------------------------*/
static ATX_Result
Properties_DeleteProperty(ATX_Properties* _self, ATX_CString name)
{
    Properties*   self = ATX_SELF(Properties, ATX_Properties);
    PropertyNode* node;
    PropertyNode* prev;

    /* find and remove the property */
    node = self->property_nodes;
    prev = NULL;
    while (node) {
        if (ATX_StringsEqual(node->property.name, name)) {
            /* match */
            if (prev) {
                prev->next = node->next;
            } else {
                self->property_nodes = node->next;
            }
    
            /* notify the listeners */
            Properties_NotifyListeners(self, 
                                       name, 
                                       NULL);

            /* destroy the node */
            PropertyNode_Destroy(node);
            return ATX_SUCCESS;
        }
        prev = node;
        node = node->next;
    }
    
    return ATX_ERROR_NO_SUCH_PROPERTY;
}

/*----------------------------------------------------------------------
|   Properties_SetProperty
+---------------------------------------------------------------------*/
ATX_METHOD
Properties_SetProperty(ATX_Properties*          _self,
                       ATX_CString              name,
                       const ATX_PropertyValue* value)
{
    Properties*   self = ATX_SELF(Properties, ATX_Properties);
    PropertyNode* node;

    /* check parameters */
    if (self == NULL || name == NULL) {
        return ATX_ERROR_INVALID_PARAMETERS;
    }

    /* special case when the value is NULL, we delete the property */
    if (value == NULL) {
        return Properties_DeleteProperty(_self, name);
    }

    /* from here one we need a value */
    if (value == NULL) {
        return ATX_ERROR_INVALID_PARAMETERS;
    }

    /* find the property with that name */
    node = Properties_FindProperty(self, name);
    if (node) {
        ATX_PropertyValue_Destruct(&node->property.value);
        PropertyNode_SetValue(node, value);
    } else {
        /* no property with that name, create one */
        node = PropertyNode_Create(name, value);
        if (node == NULL) return ATX_ERROR_OUT_OF_MEMORY;

        /* add the node to the list */
        node->next = self->property_nodes;
        self->property_nodes = node;
    } 

    /* notify the listeners */
    Properties_NotifyListeners(self, name, value);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   Properties_Clear
+---------------------------------------------------------------------*/
ATX_METHOD
Properties_Clear(ATX_Properties* _self)
{
    Properties*   self = ATX_SELF(Properties, ATX_Properties);
    PropertyNode* property_node;

    /* notify the listeners */
    PropertyListenerNode* node = self->listener_nodes;
    while (node) {
        ATX_PropertyListener_OnPropertyChanged(node->listener, 
                                               node->name, 
                                               NULL);
        node = node->next;
    }

    /* free property nodes */
    property_node = self->property_nodes;
    while (property_node) {
        PropertyNode* next = property_node->next;
        PropertyNode_Destroy(property_node);
        property_node = next;
    }

    /* clear the head */
    self->property_nodes = NULL;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   Properties_GetIterator
+---------------------------------------------------------------------*/
ATX_METHOD
Properties_GetIterator(ATX_Properties* _self,
                       ATX_Iterator**  iterator)
{
    Properties* self = ATX_SELF(Properties, ATX_Properties);

    return PropertiesIterator_Create(self, iterator);
}

/*----------------------------------------------------------------------
|   Properties_AddListener
+---------------------------------------------------------------------*/
ATX_METHOD
Properties_AddListener(ATX_Properties*             _self,
                       ATX_CString                 name,
                       ATX_PropertyListener*       listener, 
                       ATX_PropertyListenerHandle* handle)
{
    Properties*           self = ATX_SELF(Properties, ATX_Properties);
    PropertyListenerNode* node;

    /* create a new listener node */
    node = PropertyListenerNode_Create(name, listener);
    if (node == NULL) return ATX_ERROR_OUT_OF_MEMORY;

    /* add the listener to the list */
    node->next = self->listener_nodes;
    self->listener_nodes = node;

    /* return a handle */
    if (handle) *handle = (ATX_PropertyListenerHandle*)node;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   Properties_RemoveListener
+---------------------------------------------------------------------*/
ATX_METHOD
Properties_RemoveListener(ATX_Properties*            _self,
                          ATX_PropertyListenerHandle handle)
{
    Properties*           self = ATX_SELF(Properties, ATX_Properties);
    PropertyListenerNode* node;
    PropertyListenerNode* prev;

    /* look for the node with this handle */
    node = self->listener_nodes;
    prev = NULL;
    while (node) {
        if (node == (PropertyListenerNode*)handle) {
            /* match */
            if (prev) {
                prev->next = node->next;
            } else {
                self->listener_nodes = node->next;
            }
            PropertyListenerNode_Destroy(node);
            return ATX_SUCCESS;
        }
        prev = node;
        node = node->next;
    }

    return ATX_ERROR_NO_SUCH_LISTENER;
}

/*----------------------------------------------------------------------
|   ATX_PropertyValue_Clone
+---------------------------------------------------------------------*/
ATX_Result 
ATX_PropertyValue_Clone(const ATX_PropertyValue* self, ATX_PropertyValue* clone)
{
    clone->type = self->type;
    switch (self->type) {
      case ATX_PROPERTY_VALUE_TYPE_INTEGER:
      case ATX_PROPERTY_VALUE_TYPE_FLOAT:
      case ATX_PROPERTY_VALUE_TYPE_BOOLEAN:
      case ATX_PROPERTY_VALUE_TYPE_POINTER:
        clone->data = self->data;
        break;

      case ATX_PROPERTY_VALUE_TYPE_STRING:
        clone->data.string = ATX_DuplicateString(self->data.string);
        break;

      case ATX_PROPERTY_VALUE_TYPE_RAW_DATA:
        clone->data.raw_data.size = self->data.raw_data.size;
        clone->data.raw_data.data = ATX_AllocateMemory(self->data.raw_data.size);
        if (clone->data.raw_data.data == NULL) {
            return ATX_ERROR_OUT_OF_MEMORY;
        }
        ATX_CopyMemory(clone->data.raw_data.data,
                       self->data.raw_data.data,
                       self->data.raw_data.size);
        break;
    }
    
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_PropertyValue_Destruct
+---------------------------------------------------------------------*/
ATX_Result
ATX_PropertyValue_Destruct(ATX_PropertyValue* self)
{
    switch (self->type) {
      case ATX_PROPERTY_VALUE_TYPE_STRING:
        ATX_FreeMemory((void*)self->data.string);
        break;

      case ATX_PROPERTY_VALUE_TYPE_RAW_DATA:
        if (self->data.raw_data.data) {
            ATX_FreeMemory(self->data.raw_data.data);
        }
        break;

      default:
        break;
    }
    
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_Property_Clone
+---------------------------------------------------------------------*/
ATX_Result
ATX_Property_Clone(const ATX_Property* self, ATX_Property* clone)
{
    if (self->name) {
        clone->name = ATX_DuplicateString(self->name);
    } else {
        clone->name = NULL;
    }
    return ATX_PropertyValue_Clone(&self->value, &clone->value);
}

/*----------------------------------------------------------------------
|   ATX_Property_Destruct
+---------------------------------------------------------------------*/
ATX_Result
ATX_Property_Destruct(ATX_Property* self)
{
    if (self->name) ATX_FreeMemory((void*)self->name);
    self->name = NULL;
    return ATX_PropertyValue_Destruct(&self->value);
}

/*----------------------------------------------------------------------
|   ATX_Properties_Create
+---------------------------------------------------------------------*/
ATX_Result 
ATX_Properties_Create(ATX_Properties** object)
{
    Properties* properties;

    /* allocate the object */
    properties = (Properties*)ATX_AllocateMemory(sizeof(Properties));
    if (properties == NULL) {
        *object = NULL;
        return ATX_ERROR_OUT_OF_MEMORY;
    }

    /* construct the object */
    properties->property_nodes = NULL;
    properties->listener_nodes = NULL;

    /* setup the interfaces */
    ATX_SET_INTERFACE(properties, Properties, ATX_Properties);
    ATX_SET_INTERFACE(properties, Properties, ATX_Destroyable);
    *object = &ATX_BASE(properties, ATX_Properties);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   Properties_Destroy
+---------------------------------------------------------------------*/
ATX_METHOD
Properties_Destroy(ATX_Destroyable* _self)
{
    Properties*           self = ATX_SELF(Properties, ATX_Destroyable);
    PropertyNode*         property_node;
    PropertyListenerNode* listener_node;

    /* free property nodes */
    property_node = self->property_nodes;
    while (property_node) {
        PropertyNode* next = property_node->next;
        PropertyNode_Destroy(property_node);
        property_node = next;
    }

    /* free the listener nodes */
    listener_node = self->listener_nodes;
    while (listener_node) {
        PropertyListenerNode* next = listener_node->next;
        PropertyListenerNode_Destroy(listener_node);
        listener_node = next;
    } 

    /* free the object */
    ATX_FreeMemory((void*)self);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(Properties) 
    ATX_GET_INTERFACE_ACCEPT(Properties, ATX_Properties)
    ATX_GET_INTERFACE_ACCEPT(Properties, ATX_Destroyable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   ATX_Destroyable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_DESTROYABLE_INTERFACE(Properties)

/*----------------------------------------------------------------------
|   ATX_Properties interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(Properties, ATX_Properties)
    Properties_GetProperty,
    Properties_SetProperty,
    Properties_Clear,
    Properties_GetIterator,
    Properties_AddListener,
    Properties_RemoveListener
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_BaseProperties_SetProperty
+---------------------------------------------------------------------*/
ATX_Result
ATX_BaseProperties_SetProperty(ATX_Properties*          self, 
                               ATX_CString              name,
                               const ATX_PropertyValue* value)
{
    ATX_COMPILER_UNUSED(self);
    ATX_COMPILER_UNUSED(name);
    ATX_COMPILER_UNUSED(value);
    return ATX_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   ATX_BaseProperties_Clear
+---------------------------------------------------------------------*/
ATX_Result
ATX_BaseProperties_Clear(ATX_Properties* self)
{
    ATX_COMPILER_UNUSED(self);
    return ATX_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   ATX_BaseProperties_GetIterator
+---------------------------------------------------------------------*/
ATX_Result
ATX_BaseProperties_GetIterator(ATX_Properties* self, ATX_Iterator** iterator)
{
    ATX_COMPILER_UNUSED(self);
    *iterator = NULL;
    return ATX_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   ATX_BaseProperties_AddListener
+---------------------------------------------------------------------*/
ATX_Result
ATX_BaseProperties_AddListener(ATX_Properties*             self,
                               ATX_CString                 name,
                               ATX_PropertyListener*       listener, 
                               ATX_PropertyListenerHandle* handle)
{
    ATX_COMPILER_UNUSED(self);
    ATX_COMPILER_UNUSED(name);
    ATX_COMPILER_UNUSED(listener);
    *handle = 0;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_BaseProperties_RemoveListener
+---------------------------------------------------------------------*/
ATX_Result
ATX_BaseProperties_RemoveListener(ATX_Properties*            self,
                                  ATX_PropertyListenerHandle handle)
{
    ATX_COMPILER_UNUSED(self);
    ATX_COMPILER_UNUSED(handle);
    
    return ATX_SUCCESS;
}









