/*****************************************************************
|
|   Atomix - JSON
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

#ifndef _ATX_JSON_H_
#define _ATX_JSON_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "AtxTypes.h"
#include "AtxDefs.h"
#include "AtxResults.h"
#include "AtxUtils.h"
#include "AtxInterfaces.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct ATX_Json ATX_Json;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
typedef enum {
    ATX_JSON_TYPE_OBJECT = 0,
    ATX_JSON_TYPE_STRING,
    ATX_JSON_TYPE_NUMBER,
    ATX_JSON_TYPE_ARRAY,
    ATX_JSON_TYPE_BOOLEAN,
    ATX_JSON_TYPE_NULL
} ATX_JsonType;

/*----------------------------------------------------------------------
|    prototypes
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

ATX_Json*         ATX_Json_CreateArray(void);
ATX_Json*         ATX_Json_CreateObject(void);
ATX_Json*         ATX_Json_CreateString(const char* value);
ATX_Json*         ATX_Json_CreateNumber(double number);
ATX_Json*         ATX_Json_CreateBoolean(ATX_Boolean value);
ATX_Json*         ATX_Json_CreateNull(void);
void              ATX_Json_Destroy(ATX_Json* self);
ATX_Json*         ATX_Json_GetChild(ATX_Json* self, const char* name);
ATX_Json*         ATX_Json_GetChildAt(ATX_Json* self, ATX_Ordinal indx);
ATX_Cardinal      ATX_Json_GetChildCount(ATX_Json* self);
ATX_Json*         ATX_Json_GetParent(ATX_Json* self);
ATX_Result        ATX_Json_AddChild(ATX_Json* self, ATX_Json* child);
const ATX_String* ATX_Json_GetName(ATX_Json* self);
ATX_JsonType      ATX_Json_GetType(ATX_Json* self);
ATX_Int32         ATX_Json_AsInteger(ATX_Json* self);
double            ATX_Json_AsDouble(ATX_Json* self);
ATX_Boolean       ATX_Json_AsBoolean(ATX_Json* self);
const ATX_String* ATX_Json_AsString(ATX_Json* self);

ATX_Result        ATX_Json_Parse(const char* serialized, ATX_Json** json);
ATX_Result        ATX_Json_ParseBuffer(const char* serialized, ATX_Size size, ATX_Json** json);
ATX_Result        ATX_Json_Serialize(ATX_Json* self, ATX_String* buffer, ATX_Boolean pretty);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ATX_JSON_H_ */
