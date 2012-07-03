/*****************************************************************
|
|      Atomix Tests - Containers
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
|       includes
+---------------------------------------------------------------------*/
#include "Atomix.h"

/*----------------------------------------------------------------------
|       macros
+---------------------------------------------------------------------*/
#define SHOULD_SUCCEED(r)                                   \
    do {                                                    \
        ATX_Result x = r;                                   \
        if (ATX_FAILED(x)) {                                \
            ATX_Debug("failed line %d (%d)\n", __LINE__, x);\
            ATX_ASSERT(0);                                  \
        }                                                   \
    } while(0)                                         

#define SHOULD_FAIL(r)                                                  \
    do {                                                                \
        ATX_Result x = r;                                               \
        if (ATX_SUCCEEDED(x)) {                                         \
            ATX_Debug("should have failed line %d (%d)\n", __LINE__, r);\
            ATX_ASSERT(0);                                              \
        }                                                               \
    } while(0)                                  

/*----------------------------------------------------------------------
|       globals
+---------------------------------------------------------------------*/
static ATX_Cardinal ItemCount = 0;

/*----------------------------------------------------------------------
|       CreateData
+---------------------------------------------------------------------*/
static void* CreateData(const char* value)
{
    ++ItemCount;
    return (void*)ATX_DuplicateString(value);
}

/*----------------------------------------------------------------------
|       DestroyData
+---------------------------------------------------------------------*/
static void DestroyData(ATX_ListDataDestructor* self, ATX_Any data, ATX_UInt32 type)
{
    ATX_COMPILER_UNUSED(self);
    ATX_COMPILER_UNUSED(type);
    ATX_ASSERT(ItemCount != 0);
    ItemCount--;
    ATX_FreeMemory(data);
}

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int 
main(int argc, char** argv)
{
    ATX_List* list;
    ATX_ListItem* item;
    ATX_Any data;
    ATX_ListDataDestructor des = {
        NULL, 
        DestroyData
    };

    ATX_COMPILER_UNUSED(argc);
    ATX_COMPILER_UNUSED(argv);

    SHOULD_SUCCEED(ATX_List_CreateEx(&des, &list));
    ATX_ASSERT(ATX_List_GetItemCount(list) == 0);
    SHOULD_SUCCEED(ATX_List_AddData(list, CreateData("hello")));
    ATX_ASSERT(ATX_List_GetItemCount(list) == 1);
    ATX_ASSERT(ItemCount == 1);

    SHOULD_SUCCEED(ATX_List_AddTypedData(list, data=CreateData("bla"), 2));
    ATX_ASSERT(ItemCount == 2);

    item = ATX_List_GetFirstItem(list);
    ATX_ASSERT(item != NULL);
    ATX_ASSERT(ATX_ListItem_GetType(item) == 0);

    item = ATX_List_GetLastItem(list);
    ATX_ASSERT(item != NULL);
    ATX_ASSERT(ATX_ListItem_GetType(item) == 2);

    item = ATX_ListItem_GetPrev(item);
    ATX_ASSERT(item == ATX_List_GetFirstItem(list));

    ATX_ASSERT(ATX_ListItem_GetPrev(item) == NULL);

    item = ATX_List_FindData(list, data);
    ATX_ASSERT(item != NULL);

    data = (void*)"sdfsdf";
    item = ATX_List_FindData(list, data);
    ATX_ASSERT(item == NULL);

    item = ATX_List_CreateItem(list);
    ATX_ListItem_SetData(item, CreateData("coucou"));
    ATX_ListItem_SetType(item, 3);
    SHOULD_SUCCEED(ATX_List_InsertItem(list, ATX_List_GetFirstItem(list), item));
    ATX_ASSERT(item == ATX_List_GetFirstItem(list));

    item = ATX_List_CreateItem(list);
    ATX_ListItem_SetData(item, CreateData("caca"));
    ATX_ListItem_SetType(item, 4);
    SHOULD_SUCCEED(ATX_List_InsertItem(list, ATX_List_GetLastItem(list), item));
    ATX_ASSERT(ATX_ListItem_GetPrev(ATX_List_GetLastItem(list)) == item);

    item = ATX_List_CreateItem(list);
    ATX_ListItem_SetData(item, CreateData("boufou"));
    ATX_ListItem_SetType(item, 5);
    SHOULD_SUCCEED(ATX_List_InsertItem(list, NULL, item));
    ATX_ASSERT(ATX_List_GetLastItem(list) == item);

    SHOULD_FAIL(ATX_List_RemoveData(list, (void*)"ggg"));

    data = ATX_ListItem_GetData(ATX_List_GetItem(list, 2));
    SHOULD_SUCCEED(ATX_List_RemoveData(list, data));
    ATX_ASSERT(ATX_List_GetItemCount(list) == 4);

    ATX_ASSERT(ATX_List_GetItem(list, 7) == NULL);

    ATX_ASSERT(ItemCount == 4);

    ATX_List_Destroy(list);
    ATX_ASSERT(ItemCount == 0);

    return 0;
}

