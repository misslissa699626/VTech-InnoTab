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

#ifndef _ATX_MAP_H_
#define _ATX_MAP_H_

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "AtxTypes.h"
#include "AtxDefs.h"
#include "AtxResults.h"
#include "AtxUtils.h"
#include "AtxList.h"

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct ATX_Map ATX_Map;
typedef struct ATX_MapEntry ATX_MapEntry;

typedef struct {
    ATX_Boolean is_set;
    ATX_Any     data;
    ATX_UInt32  type;
} ATX_MapEntryInfo;

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define ATX_MAP_ITEM_TYPE_UNKNOWN 0

/*----------------------------------------------------------------------
|    prototypes
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

ATX_Result    ATX_Map_Create(ATX_Map** map);
ATX_Result    ATX_Map_CreateEx(const ATX_ListDataDestructor* destructor, ATX_Map** map);
ATX_Result    ATX_Map_Destroy(ATX_Map* self);
ATX_Result    ATX_Map_Clear(ATX_Map* self);
ATX_Result    ATX_Map_Put(ATX_Map*          self, 
                          ATX_CString       key, 
                          ATX_Any           data, 
                          ATX_MapEntryInfo* previous);
ATX_Result    ATX_Map_PutTyped(ATX_Map*          self, 
                               ATX_CString       key, 
                               ATX_Any           data, 
                               ATX_UInt32        type,
                               ATX_MapEntryInfo* previous);
ATX_MapEntry* ATX_Map_Get(ATX_Map* self, const char* key);
ATX_Result    ATX_Map_Remove(ATX_Map* self, ATX_CString key, ATX_MapEntryInfo* entry_info);
ATX_Boolean   ATX_Map_HasKey(ATX_Map* self, ATX_CString key);
ATX_List*     ATX_Map_AsList(ATX_Map* self);

ATX_CString   ATX_MapEntry_GetKey(ATX_MapEntry* self);
ATX_Any       ATX_MapEntry_GetData(ATX_MapEntry* self);
ATX_Result    ATX_MapEntry_SetData(ATX_MapEntry* self, ATX_Any data);
ATX_UInt32    ATX_MapEntry_GetType(ATX_MapEntry* self);
ATX_Result    ATX_MapEntry_SetType(ATX_MapEntry* self, ATX_UInt32 type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ATX_MAP_H_ */
