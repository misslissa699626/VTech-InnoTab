/*****************************************************************
|
|   Atomix - Linked Lists
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

#if !defined(_ATX_LIST_FRIEND_INCLUDE_)
/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "AtxConfig.h"
#include "AtxTypes.h"
#include "AtxDefs.h"
#include "AtxResults.h"
#include "AtxUtils.h"
#include "AtxList.h"
#include "AtxReferenceable.h"

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
#endif /* _ATX_LIST_FRIEND_INCLUDE_ */

struct ATX_ListItem {
    ATX_Any       data;
    ATX_UInt32    type;
    ATX_ListItem* next;
    ATX_ListItem* prev;
};


struct ATX_List {
    ATX_Cardinal           item_count;
    ATX_ListItem*          head;
    ATX_ListItem*          tail;
    ATX_ListDataDestructor destructor;
};

#if !defined(_ATX_LIST_FRIEND_INCLUDE_)

/*----------------------------------------------------------------------
|    ATX_List_Create
+---------------------------------------------------------------------*/
ATX_Result 
ATX_List_Create(ATX_List** list)
{
    return ATX_List_CreateEx(NULL, list);
}

/*----------------------------------------------------------------------
|    ATX_List_CreateEx
+---------------------------------------------------------------------*/
ATX_Result 
ATX_List_CreateEx(const ATX_ListDataDestructor* destructor, ATX_List** list)
{
    /* allocate memory for the object */
    *list = ATX_AllocateZeroMemory(sizeof(ATX_List));
    if (*list == NULL) {
        return ATX_ERROR_OUT_OF_MEMORY;
    }

    /* construct the objects */
    if (destructor) {
        (*list)->destructor = *destructor;
    }

    /* done */
    return ATX_SUCCESS;
}


/*----------------------------------------------------------------------
|    ATX_List_Destroy
+---------------------------------------------------------------------*/
ATX_Result
ATX_List_Destroy(ATX_List* list)
{
    if (list == NULL) return ATX_SUCCESS;

    /* destroy all items */
    ATX_List_Clear(list);

    /* destroy the list object */
    ATX_FreeMemory((void*)list);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_List_Clear
+---------------------------------------------------------------------*/
ATX_Result
ATX_List_Clear(ATX_List* list)
{
    ATX_ListItem* item = list->head;

    /* destroy all items */
    while (item) {
        ATX_ListItem* next = item->next;

        /* destroy the item data */
        if (list->destructor.DestroyData) {
            list->destructor.DestroyData(&list->destructor,
                                         item->data,
                                         item->type);
        }

        /* free the item memory */
        ATX_FreeMemory(item);

        item = next;
    }

    /* reset item count and pointers */
    list->item_count = 0;
    list->head = NULL;
    list->tail = NULL;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_List_AddItem
+---------------------------------------------------------------------*/
ATX_Result 
ATX_List_AddItem(ATX_List* list, ATX_ListItem* item)
{
    /* add the item */
    if (list->tail) {
        item->prev = list->tail;
        item->next = NULL;
        list->tail->next = item;
        list->tail = item;
    } else {
        list->head = item;
        list->tail = item;
        item->next = NULL;
        item->prev = NULL;
    }

    /* one more item in the list now */
    list->item_count++;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_List_CreateItem
+---------------------------------------------------------------------*/
ATX_ListItem*
ATX_List_CreateItem(ATX_List* list)
{
    ATX_ListItem* item;

    /* avoid compiler warning about the unused parameter */
    ATX_COMPILER_UNUSED(list);

    /* allocate a new item */
    item = (ATX_ListItem*)ATX_AllocateMemory(sizeof(ATX_ListItem));
    if (item == NULL) {
        return NULL;
    }

    /* initialize the item */
    item->data = NULL;
    item->type = 0;
    item->next = NULL;
    item->prev = NULL;

    return item;
}

/*----------------------------------------------------------------------
|    ATX_List_AddTypedData
+---------------------------------------------------------------------*/
ATX_Result 
ATX_List_AddTypedData(ATX_List* list, ATX_Any data, ATX_UInt32 type)
{
    ATX_ListItem* item;

    /* create a new item */
    item = ATX_List_CreateItem(list);
    if (item == NULL) return ATX_ERROR_OUT_OF_MEMORY;

    /* attach the data to the item */
    item->data = data;
    item->type = type;

    /* add the item to the list */
    return ATX_List_AddItem(list, item);
}

/*----------------------------------------------------------------------
|    ATX_List_AddData
+---------------------------------------------------------------------*/
ATX_Result 
ATX_List_AddData(ATX_List* list, ATX_Any data)
{
    return ATX_List_AddTypedData(list, data, ATX_LIST_ITEM_TYPE_UNKNOWN);
}

/*----------------------------------------------------------------------
|    ATX_List_DetachItem
+---------------------------------------------------------------------*/
ATX_Result
ATX_List_DetachItem(ATX_List* list, ATX_ListItem* item)
{
    if (item->prev) {
        /* item is not the head */
        if (item->next) {
            /* item is not the tail */
            item->next->prev = item->prev;
            item->prev->next = item->next;
        } else {
            /* item is the tail */
            list->tail = item->prev;
            list->tail->next = NULL;
        }
    } else {
        /* item is the head */
        list->head = item->next;
        if (list->head) {
            /* item is not the tail */
            list->head->prev = NULL;
        } else {
            /* item is also the tail */
            list->tail = NULL;
        }
    }

    /* clear the links */
    item->prev = item->next = NULL;
    
    /* one less item in the list now */
    list->item_count--;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_List_DestroyItem
+---------------------------------------------------------------------*/
ATX_Result
ATX_List_DestroyItem(ATX_List* list, ATX_ListItem* item)
{
    if (item->next || item->prev) {
        return ATX_ERROR_INVALID_STATE;
    }
    
    /* destroy the item data */
    if (list->destructor.DestroyData) {
        list->destructor.DestroyData(&list->destructor,
                                     item->data,
                                     item->type);
    }
    
    /* free the item memory */
    ATX_FreeMemory(item);    
    
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_List_InsertItem
+---------------------------------------------------------------------*/
ATX_Result
ATX_List_InsertItem(ATX_List* list, ATX_ListItem* where, ATX_ListItem* item)
{
    /* insert the item in the list */
    ATX_ListItem* position = where;
    if (position) {
        /* insert at position */
        item->next = position;
        item->prev = position->prev;
        position->prev = item;
        if (item->prev) {
            item->prev->next = item;
        } else {
            /* this is the new head */
            list->head = item;
        }

        /* one more item in the list now */
        ++list->item_count;
    } else {
        /* insert at tail */
        return ATX_List_AddItem(list, item);
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_List_InsertData
+---------------------------------------------------------------------*/
ATX_Result
ATX_List_InsertData(ATX_List* list, ATX_ListItem* where, ATX_Any data)
{
    return ATX_List_InsertTypedData(list, 
                                    where, 
                                    data, 
                                    ATX_LIST_ITEM_TYPE_UNKNOWN);
}

/*----------------------------------------------------------------------
|   ATX_List_InsertTypedData
+---------------------------------------------------------------------*/
ATX_Result
ATX_List_InsertTypedData(ATX_List*     list, 
                         ATX_ListItem* where, 
                         ATX_Any       data, 
                         ATX_UInt32    type)
{
    /* create a new item */
    ATX_ListItem* item = ATX_List_CreateItem(list);
    if (item == NULL) return ATX_ERROR_OUT_OF_MEMORY;

    /* setup the item */
    item->data = data;
    item->type = type;

    /* insert the item */
    return ATX_List_InsertItem(list, where, item);
}

/*----------------------------------------------------------------------
|    ATX_List_RemoveItem
+---------------------------------------------------------------------*/
ATX_Result
ATX_List_RemoveItem(ATX_List* list, ATX_ListItem* item)
{
    /* pop the item out of the list */
    ATX_List_DetachItem(list, item);

    /* destroy the item */
    return ATX_List_DestroyItem(list, item);
}

/*----------------------------------------------------------------------
|    ATX_List_RemoveData
+---------------------------------------------------------------------*/
ATX_Result    
ATX_List_RemoveData(ATX_List* list, ATX_Any data)
{
    ATX_ListItem* item = list->head;

    while (item) {
        if (item->data == data) {
            return ATX_List_RemoveItem(list, item);
        }
        item = item->next;
    }

    return ATX_ERROR_NO_SUCH_ITEM;
}

/*----------------------------------------------------------------------
|    ATX_List_GetFirstItem
+---------------------------------------------------------------------*/
ATX_ListItem*
ATX_List_GetFirstItem(ATX_List* list)
{
    return list->head;
}

/*----------------------------------------------------------------------
|    ATX_List_GetLastItem
+---------------------------------------------------------------------*/
ATX_ListItem*
ATX_List_GetLastItem(ATX_List* list)
{
    return list->tail;
}

/*----------------------------------------------------------------------
|    ATX_List_GetItem
+---------------------------------------------------------------------*/
ATX_ListItem*
ATX_List_GetItem(ATX_List* list, ATX_Ordinal indx)
{
    ATX_ListItem* item = list->head;
    
    /* check the range */
    if (indx >= list->item_count) return NULL;

    /* advance to the requested item */
    while (indx--) item = item->next;

    return item;
}

/*----------------------------------------------------------------------
|    ATX_List_GetItemCount
+---------------------------------------------------------------------*/
ATX_Cardinal  
ATX_List_GetItemCount(ATX_List* list)
{
    return list->item_count;
}

/*----------------------------------------------------------------------
|    ATX_List_Find
+---------------------------------------------------------------------*/
ATX_ListItem* 
ATX_List_Find(ATX_List* list, const ATX_ListDataPredicate* predicate)
{
    ATX_ListItem* item = list->head;

    while (item) {
        if (predicate->Evaluate(predicate, item->data, item->type)) {
            return item;
        }
        item = item->next;
    }

    return NULL;
}

/*----------------------------------------------------------------------
|    ATX_List_FindData
+---------------------------------------------------------------------*/
ATX_ListItem* 
ATX_List_FindData(ATX_List* list, ATX_Any data)
{
    ATX_ListItem* item = list->head;

    while (item) {
        if (item->data == data) {
            /* match, return the item */
            return item;
        }
        item = item->next;
    }

    return NULL;
}

/*----------------------------------------------------------------------
|    ATX_ListItem_GetData
+---------------------------------------------------------------------*/
ATX_Any
ATX_ListItem_GetData(ATX_ListItem* item)
{
    return item->data;
}

/*----------------------------------------------------------------------
|    ATX_ListItem_SetData
+---------------------------------------------------------------------*/
ATX_Result
ATX_ListItem_SetData(ATX_ListItem* item, ATX_Any data)
{
    item->data = data;
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_ListItem_GetType
+---------------------------------------------------------------------*/
ATX_UInt32
ATX_ListItem_GetType(ATX_ListItem* item)
{
    return item->type;
}

/*----------------------------------------------------------------------
|    ATX_ListItem_SetType
+---------------------------------------------------------------------*/
ATX_Result
ATX_ListItem_SetType(ATX_ListItem* item, ATX_UInt32 type)
{
    item->type = type;
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_ListItem_GetNext
+---------------------------------------------------------------------*/
ATX_ListItem* 
ATX_ListItem_GetNext(ATX_ListItem* item)
{
    return item->next;
}

/*----------------------------------------------------------------------
|    ATX_ListItem_GetPrev
+---------------------------------------------------------------------*/
ATX_ListItem* 
ATX_ListItem_GetPrev(ATX_ListItem* item)
{
    return item->prev;
}

#endif /* _ATX_LIST_FRIEND_INCLUDE_ */
