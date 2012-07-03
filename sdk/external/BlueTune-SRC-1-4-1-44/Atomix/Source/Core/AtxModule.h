/*****************************************************************
|
|   Atomix - Module Interface
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
 * Header file for the ATX_Module interface 
 */

#ifndef _ATX_MODULE_H_
#define _ATX_MODULE_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "AtxDefs.h"
#include "AtxTypes.h"
#include "AtxInterfaces.h"
#include "AtxProperties.h"

/*----------------------------------------------------------------------
|   ATX_Module interface
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(ATX_Module)
/**
 * @brief Interface implemented by objects that create other objects
 *  
 * A Module object is responsible for creating object instance of a certain 
 * class. Module objects implement the ATX_Module interface, and clients that
 * want to create instances of that module will call the CreateObject method.
 */
ATX_BEGIN_INTERFACE_DEFINITION(ATX_Module)
    /** create an instance of the module that implements a given interface
     * @param self Pointer to the object on which the method is called 
     * @param parameters Generic parameters used for constructing the object
     * @param interface_id Interface ID that the object needs to implement
     * @param object address of an object reference where the created object
     * will be returned if the call succeeds
     * @atx_method_result
     */
    ATX_Result (*CreateInstance)(ATX_Module*            self,
                                 ATX_String             parameters,
                                 const ATX_InterfaceId* interface_id,
                                 ATX_Object*            object);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
/** 
 * Convenience macro for calling the CreateObject method of the 
 * ATX_Module interface.
 */
#define ATX_Module_CreateObject(self, parameters, interface_id, result)   \
ATX_INTERFACE(object)->CreateObject(self,                                 \
                                    parameters,                           \
                                    interface_id,                         \
                                    result)

/*----------------------------------------------------------------------
|   macros and templates
+---------------------------------------------------------------------*/
#define ATX_DEFINE_NULL_MODULE_INSTANCE(_prefix) \
static ATX_ModuleInstance* const _prefix##ModuleInstance = NULL;

#define ATX_IMPLEMENT_SIMPLE_MODULE_INTERFACE(_refix)                        \
static const ATX_ModuleInterface _prefix##ModuleATX_ModuleInterface = {      \
    _prefix##Module_GetInterface,                                            \
    _prefix##Module_CreateInstance                                           \
};                                                                            

#define ATX_IMPLEMENT_SIMPLE_MODULE_PROPERTIES(_prefix)                    \
ATX_DECLARE_SIMPLE_GET_INTERFACE_IMPLEMENTATION(_prefix##Module)           \
ATX_IMPLEMENT_SIMPLE_STATIC_PROPERTIES_INTERFACE(_prefix##Module,          \
                                                 _prefix##ModuleProperties)

#endif /* _ATX_MODULE_H_ */




