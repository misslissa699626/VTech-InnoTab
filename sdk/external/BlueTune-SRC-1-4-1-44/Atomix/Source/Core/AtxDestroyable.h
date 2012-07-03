/*****************************************************************
|
|   Atomix - Destroyable Interface
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
 * Header file for the ATX_Destroyable interface 
*/

#ifndef _ATX_DESTROYABLE_H_
#define _ATX_DESTROYABLE_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "AtxInterfaces.h"
#include "AtxDefs.h"
#include "AtxTypes.h"
#include "AtxDebug.h"

/*----------------------------------------------------------------------
|   ATX_Destroyable interface
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(ATX_Destroyable)
/**
 * Interface implemented by objects that can be destroyed.
 */
ATX_BEGIN_INTERFACE_DEFINITION(ATX_Destroyable)
    /**
     * Destroys the object instance.
     * After destroying an instance, that instance can no longer be used
     * (clients can no longer call any of the methods on that instance)
     * @param self Pointer the the object on which this method 
     * is called
     * @atx_method_result
     */
    ATX_Result (*Destroy)(ATX_Destroyable* self);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   interface stubs
+---------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif

ATX_Result ATX_Destroyable_Destroy(ATX_Destroyable* self);

#if defined(__cplusplus)
}
#endif

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
/**
 * Macro to destroy an object through the ATX_Destroyable interface
 *
 * This macro will first try to get an ATX_Destroyable intrerface for an
 * object. If the object does not implement the ATX_Destroyable interface,
 * an exception is thrown. If the object implements the ATX_Destroyble interface,
 * the macro calls the Destroy() method.
 * As a side effect, this macro clears its object reference argument, 
 * making it a NULL object reference.
 */
#define ATX_DESTROY_OBJECT(object)                                        \
do {                                                                      \
    if (object) {                                                         \
        ATX_Destroyable* destroyable = ATX_CAST(object, ATX_Destroyable); \
        ATX_ASSERT(destroyable != NULL);                                  \
        ATX_Destroyable_Destroy(destroyable);                             \
        object = NULL;                                                    \
    }                                                                     \
} while(0)  

#define ATX_IMPLEMENT_DESTROYABLE_INTERFACE(_class)            \
ATX_IMPLEMENT_GET_INTERFACE_ADAPTER(_class, ATX_Destroyable)   \
static const ATX_DestroyableInterface                          \
_class##_ATX_DestroyableInterface = {                          \
    ATX_GET_INTERFACE_ADAPTER(_class, ATX_Destroyable),        \
    _class##_Destroy                                           \
};         
 
#endif /* _ATX_DESTROYABLE_H_ */





