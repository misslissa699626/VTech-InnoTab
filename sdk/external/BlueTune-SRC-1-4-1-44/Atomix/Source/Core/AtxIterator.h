/*****************************************************************
|
|   Atomix - Iterator Interface
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
 * Header file for the ATX_Iterator interface 
 */

#ifndef _ATX_ITERATOR_H_
#define _ATX_ITERATOR_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "AtxInterfaces.h"
#include "AtxTypes.h"

/*----------------------------------------------------------------------
|   error codes
+---------------------------------------------------------------------*/
#define ATX_ERROR_END_OF_ITERATION (ATX_ERROR_BASE_ITERATOR - 0)

/*----------------------------------------------------------------------
|   ATX_Iterator interface
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(ATX_Iterator)
/**
 * Interface implemented by objects that represent a list of
 * elements of arbitrary type.
 *
 * An ATX_Iterator interface is used to enumerate elements in a list.
 * Clients of that interface typically enumerate elements by
 * calling the GetNext() method repeatedly until they have found the
 * element they were looking for, or the end of the list has been reached.
 */
ATX_BEGIN_INTERFACE_DEFINITION(ATX_Iterator)
    /**
     * Returns the next element in the list, or an error code when the end
     * of the list has been reached.
     * @param self Pointer to the object on which this method 
     * is called
     * @param element Pointer to a variable that where the next element will
     * be returned if the end of the list has not yet been reached
     * @return ATX_SUCCESS if an element has been returned, 
     * ATX_ERROR_END_OF_ITERATION if the end of the list has been reached, or
     * a negative error code if an error occurs.
     */
    ATX_Result (*GetNext)(ATX_Iterator* self, ATX_Any* element);

    /**
     * Resets the iterator, making it go back to the first element in the list.
     * After a call to Reset(), a call to GetNext() will return the first 
     * element in the list.
     * @param self Pointer to the object on which this method 
     * is called
     * @atx_method_result
     */
    ATX_Result (*Reset)(ATX_Iterator* self);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
/**
 * Convenience macro used to call the GetNext() method on objects that 
 * implement the ATX_Iterator interface 
 */
#define ATX_Iterator_GetNext(self, element) \
ATX_INTERFACE(self)->GetNext(self, element)

/**
 * Convenience macro used to call the Reset() method on objects that 
 * implement the ATX_Iterator interface 
 */
#define ATX_Iterator_Reset(self) \
ATX_INTERFACE(self)->Reset(self)

#endif /* _ATX_ITERATOR_H_ */


