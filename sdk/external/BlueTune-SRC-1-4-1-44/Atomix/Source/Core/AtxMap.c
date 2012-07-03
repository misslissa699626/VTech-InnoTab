/*****************************************************************
|
|   Atomix - Maps
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
|    includes
+---------------------------------------------------------------------*/
#include "AtxConfig.h"
#include "AtxTypes.h"
#include "AtxDefs.h"
#include "AtxResults.h"
#include "AtxUtils.h"
#include "AtxMap.h"
#include "AtxString.h"

#define _ATX_LIST_FRIEND_INCLUDE_
#include "AtxList.c"
#undef _ATX_LIST_FRIEND_INCLUDE_

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
struct ATX_MapEntry {
    ATX_ListItem base;
    ATX_String   key;
};

struct ATX_Map {
    ATX_List entries;
};

/*----------------------------------------------------------------------
|    ATX_Map_Create
+---------------------------------------------------------------------*/
ATX_Result 
ATX_Map_Create(ATX_Map** map)
{
    return ATX_Map_CreateEx(NULL, map);
}

/*----------------------------------------------------------------------
|    ATX_Map_CreateEx
+---------------------------------------------------------------------*/
ATX_Result 
ATX_Map_CreateEx(const ATX_ListDataDestructor* destructor, ATX_Map** map)
{
    return ATX_List_CreateEx(destructor, (ATX_List**)map);
}

/*----------------------------------------------------------------------
|    ATX_Map_Destroy
+---------------------------------------------------------------------*/
ATX_Result
ATX_Map_Destroy(ATX_Map* self)
{
    /* clear all the enties */
    ATX_Map_Clear(self);

    /* destroy the object */
    return ATX_List_Destroy((ATX_List*)self);
}

/*----------------------------------------------------------------------
|    ATX_Map_Clear
+---------------------------------------------------------------------*/
ATX_Result
ATX_Map_Clear(ATX_Map* self)
{
    /* destroy all the enties */
    ATX_ListItem* item = self->entries.head;
    while (item) {
        ATX_MapEntry* entry = (ATX_MapEntry*)item;

        /* destroy the key */
        ATX_String_Destruct(&entry->key);

        item = item->next;
    }

    return ATX_List_Clear((ATX_List*)self);
}

/*----------------------------------------------------------------------
|    ATX_Map_PutTyped
+---------------------------------------------------------------------*/
ATX_Result 
ATX_Map_PutTyped(ATX_Map*          self, 
                 ATX_CString       key, 
                 ATX_Any           data, 
                 ATX_UInt32        type,
                 ATX_MapEntryInfo* previous)
{
    /* check if the entry already exists */
    ATX_MapEntry* entry = ATX_Map_Get(self, key);
    if (entry) {
        if (previous) {
            /* return the previous entry */
            previous->is_set = ATX_TRUE;
            previous->data   = entry->base.data;
            previous->type   = entry->base.type;
        } else {
            /* destroy the previous entry */
            if (self->entries.destructor.DestroyData) {
                self->entries.destructor.DestroyData (
                    &self->entries.destructor,
                    entry->base.data,
                    entry->base.type);
            }
        }
    } else {
        ATX_Result  result;

        if (previous)  {
            previous->is_set = ATX_FALSE;
            previous->data   = NULL;
            previous->type   = 0;
        }

        /* allocate a new entry */
        entry = (ATX_MapEntry*)ATX_AllocateMemory(sizeof(ATX_MapEntry));
        if (entry == NULL) return ATX_ERROR_OUT_OF_MEMORY;

        /* partially initialize the entry */
        entry->key = ATX_String_Create(key);

        /* add the entry to the list */
        result = ATX_List_AddItem(&self->entries, (ATX_ListItem*)entry);
        if (ATX_FAILED(result)) {
            ATX_FreeMemory((void*)entry);
            return result;
        }
    }

    /* update/init the entry */
    entry->base.data = data;
    entry->base.type = type;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_Map_Put
+---------------------------------------------------------------------*/
ATX_Result    
ATX_Map_Put(ATX_Map*          self, 
            ATX_CString       key, 
            ATX_Any           data, 
            ATX_MapEntryInfo* previous)
{
    return ATX_Map_PutTyped(self, key, data, 0, previous);
}

/*----------------------------------------------------------------------
|    ATX_Map_Get
+---------------------------------------------------------------------*/
ATX_MapEntry* 
ATX_Map_Get(ATX_Map* self, const char* key)
{
    ATX_ListItem* item = self->entries.head;
    while (item) {
        ATX_MapEntry* entry = (ATX_MapEntry*)item;
        if (ATX_String_Equals(&entry->key, key, ATX_FALSE)) return entry;
        item = item->next;
    }

    return NULL;
}

/*----------------------------------------------------------------------
|    ATX_Map_Remove
+---------------------------------------------------------------------*/
ATX_Result    
ATX_Map_Remove(ATX_Map* self, ATX_CString key, ATX_MapEntryInfo* entry_info)
{
    ATX_ListItem* item = self->entries.head;
    while (item) {
        ATX_MapEntry* entry = (ATX_MapEntry*)item;
        if (ATX_String_Equals(&entry->key, key, ATX_FALSE)) {
            if (entry_info) {
                /* return, but do not destroy the existing entry */
                entry_info->is_set = ATX_TRUE;
                entry_info->data   = entry->base.data;
                entry_info->type   = entry->base.type;

                ATX_String_Destruct(&entry->key);
                ATX_List_DetachItem(&self->entries, item);
                ATX_FreeMemory((void*)item);
            } else {
                ATX_String_Destruct(&entry->key);
                ATX_List_RemoveItem((ATX_List*)self, item);
            }
            return ATX_SUCCESS;
        }
        item = item->next;
    }

    return ATX_ERROR_NO_SUCH_ITEM;
}

/*----------------------------------------------------------------------
|    ATX_Map_HasKey
+---------------------------------------------------------------------*/
ATX_Boolean   
ATX_Map_HasKey(ATX_Map* self, ATX_CString key)
{
    ATX_ListItem* item = self->entries.head;
    while (item) {
        ATX_MapEntry* entry = (ATX_MapEntry*)item;
        if (ATX_String_Equals(&entry->key, key, ATX_FALSE)) return ATX_TRUE;
        item = item->next;
    }

    return ATX_FALSE;
}

/*----------------------------------------------------------------------
|    ATX_Map_AsList
+---------------------------------------------------------------------*/
ATX_List*     
ATX_Map_AsList(ATX_Map* self)
{
    return (ATX_List*)self;
}

/*----------------------------------------------------------------------
|    ATX_MapEntry_GetKey
+---------------------------------------------------------------------*/
ATX_CString   
ATX_MapEntry_GetKey(ATX_MapEntry* self)
{
    return ATX_CSTR(self->key);
}

/*----------------------------------------------------------------------
|    ATX_MapEntry_GetData
+---------------------------------------------------------------------*/
ATX_Any       
ATX_MapEntry_GetData(ATX_MapEntry* self)
{
    return self->base.data;
}

/*----------------------------------------------------------------------
|    ATX_MapEntry_SetData
+---------------------------------------------------------------------*/
ATX_Result    
ATX_MapEntry_SetData(ATX_MapEntry* self, ATX_Any data)
{
    self->base.data = data;
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_MapEntry_GetType
+---------------------------------------------------------------------*/
ATX_UInt32    
ATX_MapEntry_GetType(ATX_MapEntry* self)
{
    return self->base.type;
}

/*----------------------------------------------------------------------
|    ATX_MapEntry_SetType
+---------------------------------------------------------------------*/
ATX_Result
ATX_MapEntry_SetType(ATX_MapEntry* self, ATX_UInt32 type)
{
    self->base.type = type;
    return ATX_SUCCESS;
}
