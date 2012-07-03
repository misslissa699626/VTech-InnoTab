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
/** @file 
 * Header file for the ATX_Properties interface 
 */

#ifndef _ATX_PROPERTIES_H_
#define _ATX_PROPERTIES_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "AtxInterfaces.h"
#include "AtxTypes.h"
#include "AtxIterator.h"

/*----------------------------------------------------------------------
|   error codes
+---------------------------------------------------------------------*/
#define ATX_ERROR_NO_SUCH_PROPERTY       (ATX_ERROR_BASE_PROPERTIES - 0)
#define ATX_ERROR_NO_SUCH_LISTENER       (ATX_ERROR_BASE_PROPERTIES - 1)
#define ATX_ERROR_PROPERTY_TYPE_MISMATCH (ATX_ERROR_BASE_PROPERTIES - 2)

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
/**
 * Type of data represented by the 'value' field of an ATX_Property structure.
 */
typedef enum {
    ATX_PROPERTY_VALUE_TYPE_INTEGER,  /**< The value is an integer              */
    ATX_PROPERTY_VALUE_TYPE_FLOAT,    /**< The value is a floating point number */
    ATX_PROPERTY_VALUE_TYPE_STRING,   /**< The value is a string                */
    ATX_PROPERTY_VALUE_TYPE_BOOLEAN,  /**< The value is a boolean               */
    ATX_PROPERTY_VALUE_TYPE_RAW_DATA, /**< The value is a raw data block        */
    ATX_PROPERTY_VALUE_TYPE_POINTER   /**< The value is a pointer               */
} ATX_PropertyValueType;

typedef struct {
    ATX_Size size;
    ATX_Any  data;
} ATX_PropertyRawData;

/**
 * Union of the different possible data types for the 'data' field of 
 * an ATX_PropertyValue struct.
 */
typedef union {
    const void*         pointer;  /**< A pointer                 */
    ATX_CString         string;   /**< A character string        */
    ATX_Int32           integer;  /**< An integer number         */
    ATX_Float           fp;       /**< A floating point number   */
    ATX_Boolean         boolean;  /**< A boolean value           */
    ATX_PropertyRawData raw_data; /**< A pointer to untyped data */
} ATX_PropertyValueData;

/**
 * Value of a property
 */
typedef struct {
    ATX_PropertyValueType type;
    ATX_PropertyValueData data;
} ATX_PropertyValue;

typedef const void* ATX_PropertyListenerHandle;

/**
 * A property that has a name and a value (the value can be of one of several
 * types)
 */
typedef struct {
    ATX_CString       name;  /**< Name of the property  */
    ATX_PropertyValue value; /**< Value of the property */
} ATX_Property;

/*----------------------------------------------------------------------
|   ATX_PropertyListener interface
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(ATX_PropertyListener)
ATX_BEGIN_INTERFACE_DEFINITION(ATX_PropertyListener)
    /**
     * Notify that a property has changed or deleted. 
     * @param name Name of the property that has changed. This parameter
     * may be NULL or an empty string when the notification is for the
     * deletion of all the properties in a list and that the listener is
     * listening for changes to all the properties (this way the listener
     * is not called once for each property deletion).
     * @param value Pointer to the value of the property. If the notification
     * is for the deletion of a property, this parameter is NULL. If the
     * notification is for the change of a property's value, this parameter
     * points to the new value of the property.
     */
    void (*OnPropertyChanged)(ATX_PropertyListener*    self,
                              ATX_CString              name,
                              const ATX_PropertyValue* value);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   ATX_Properties interface
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(ATX_Properties)
/**
 * Interface implemented by objects that want to expose a list of properties
 * to their clients. 
 *
 * Properties are name/value pairs (ATX_Property type), 
 * where the name is a string, and the value can be of one of several 
 * possible types (string, integer, etc...)
 */
ATX_BEGIN_INTERFACE_DEFINITION(ATX_Properties)
    /**
     * Get the value of a property by name.
     * @param self Pointer to the object on which this method is called
     * @param name Name of the property of which the value is requested.
     * @param value Pointer to where the value of the property should be
     * written.
     * @return ATX_SUCCESS if the property with the specified name is found
     * and its value returned, ATX_ERROR_NO_SUCH_PROPERY if there is no 
     * property with that name in the list, or a negative ATX_Result error 
     * code if the call fails for another reason.
     */
    ATX_Result (*GetProperty)(ATX_Properties*    self,
                              ATX_CString        name,
                              ATX_PropertyValue* value);

    /**
     * Set the value of a property in a property list.
     * @param self Pointer to the object on which this method is called.
     * @param name Name of the property to set.
     * @param value Pointer to the value of the property to set, or NULL
     * to delete the property (remove the property from the list).
     * @atx_method_result
     */
    ATX_Result (*SetProperty)(ATX_Properties*          self,
                              ATX_CString              name,
                              const ATX_PropertyValue* value);

    /**
     * Delete all properties.
     * @param self Pointer to the object on which this method is called
     * @atx_method_result
     */
    ATX_Result (*Clear)(ATX_Properties* self);

    /**
      * Get an iterator for the properties in the list. If this list cannot
      * be iterated, this method returns ATX_ERROR_NOT_SUPPORTED.
      */
    ATX_Result (*GetIterator)(ATX_Properties* self,
                              ATX_Iterator**  iterator);

    /**
     * Add a listener. The listener will notified of changes to one
     * or all properties.
     * @param name Name of the property of whose changes the listener wants
     * to be notified. If this parameter is NULL, the listener will be 
     * notified of changes to any of the properties in the list.
     * @param listener Pointer to a listener object that will receive 
     * notifications.
     * @param handle Pointer to a handle where the listener handle will
     * be returned.
     */
    ATX_Result (*AddListener)(ATX_Properties*             self,
                              ATX_CString                 name,
                              ATX_PropertyListener*       listener, 
                              ATX_PropertyListenerHandle* handle);

    ATX_Result (*RemoveListener)(ATX_Properties*            self,
                                 ATX_PropertyListenerHandle handle);

ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
/**
 * Convenience macro used to call the GetProperty() method on objects 
 * that implement the ATX_Properties interface 
 */
#define ATX_Properties_GetProperty(object, name, value)  \
ATX_INTERFACE(object)->GetProperty(object,               \
                                   name,                 \
                                   value)

/**
 * Convenience macro used to call the SetProperty() method on objects that 
 * implement the ATX_Properties interface 
 */
#define ATX_Properties_SetProperty(object, name, value)    \
ATX_INTERFACE(object)->SetProperty(object,                 \
                                   name,                   \
                                   value)

/**
 * Convenience macro used to call the Clear() method on objects that 
 * implement the ATX_Properties interface 
 */
#define ATX_Properties_Clear(object)                            \
ATX_INTERFACE(object)->Clear(object)

/**
 * Convenience macro used to call the GetIterator() method on objects that 
 * implement the ATX_Properties interface 
 */
#define ATX_Properties_GetIterator(object, iterator)               \
ATX_INTERFACE(object)->GetIterator(object, iterator)

/**
 * Convenience macro used to call the AddListener() method on objects that 
 * implement the ATX_Properties interface 
 */
#define ATX_Properties_AddListener(object, name, listener, handle) \
ATX_INTERFACE(object)->AddListener(object,                         \
                                   name,                           \
                                   listener,                       \
                                   handle)

/**
 * Convenience macro used to call the RemoveListener() method on objects that 
 * implement the ATX_Properties interface 
 */
#define ATX_Properties_RemoveListener(object, handle)    \
ATX_INTERFACE(object)->RemoveListener(object, handle)

/**
 * Convenience macro used to call the OnPropertyChanged() method on objects 
 * that implement the ATX_PropertyListener interface 
 */
#define ATX_PropertyListener_OnPropertyChanged(object, name, value) \
ATX_INTERFACE(object)->OnPropertyChanged(object,                    \
                                         name,                      \
                                         value)

/*----------------------------------------------------------------------
|   implementation templates
+---------------------------------------------------------------------*/
#define ATX_IMPLEMENT_STATIC_PROPERTIES_INTERFACE(_class) \
ATX_BEGIN_INTERFACE_MAP(_class, ATX_Properties)           \
    _class##_GetProperty,                                 \
    ATX_BaseProperties_SetProperty,                       \
    ATX_BaseProperties_Clear,                             \
    ATX_BaseProperties_GetIterator,                       \
    ATX_BaseProperties_AddListener,                       \
    ATX_BaseProperties_RemoveListener                     \
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   functions
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

ATX_Result 
ATX_BaseProperties_SetProperty(ATX_Properties*          self, 
                               ATX_CString              name,
                               const ATX_PropertyValue* value);
ATX_Result
ATX_BaseProperties_Clear(ATX_Properties* self);
ATX_Result
ATX_BaseProperties_GetIterator(ATX_Properties* self, ATX_Iterator** iterator);
ATX_Result
ATX_BaseProperties_AddListener(ATX_Properties*             self,
                               ATX_CString                 name,
                               ATX_PropertyListener*       listener, 
                               ATX_PropertyListenerHandle* handle);
ATX_Result
ATX_BaseProperties_RemoveListener(ATX_Properties*            self,
                                  ATX_PropertyListenerHandle handle);

ATX_Result ATX_Properties_Create(ATX_Properties** properties);
ATX_Result ATX_PropertyValue_Clone(const ATX_PropertyValue* self, 
                                   ATX_PropertyValue*       clone);
ATX_Result ATX_PropertyValue_Destruct(ATX_PropertyValue* self);
ATX_Result ATX_Property_Clone(const ATX_Property* self, 
                              ATX_Property*       clone);
ATX_Result ATX_Property_Destruct(ATX_Property* self);

#ifdef __cplusplus
}
#endif

#endif /* _ATX_PROPERTIES_H_ */
