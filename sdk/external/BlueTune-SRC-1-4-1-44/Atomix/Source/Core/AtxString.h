/*****************************************************************
|
|   Atomix - String Objects
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

#ifndef _ATX_STRINGS_H_
#define _ATX_STRINGS_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "AtxConfig.h"
#include "AtxTypes.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define ATX_STRING_SEARCH_FAILED (-1)
#define ATX_EMPTY_STRING {0}

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
extern const char* const ATX_String_EmptyString;
#ifdef __cplusplus
}
#endif /* __cplusplus */

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct {
    char* chars;
} ATX_String;

typedef struct {
    ATX_Cardinal length;
    ATX_Cardinal allocated;
    /* the actual string characters follow */
} ATX_StringBuffer;

/*----------------------------------------------------------------------
|   ATX_String inline functions
+---------------------------------------------------------------------*/
#define ATX_String_GetBuffer(str) ( ((ATX_StringBuffer*)((str)->chars))-1 )
#define ATX_String_Construct(str) do {                  \
    (str)->chars = NULL;                                \
} while(0)
#define ATX_String_Destruct(str) do {                        \
    if ((str)->chars) {                                      \
        ATX_FreeMemory((void*)ATX_String_GetBuffer((str)));  \
        (str)->chars = NULL;                                 \
    }                                                        \
} while(0)
#define ATX_String_GetChar(str, index) ((str)->chars[(index)])
#define ATX_String_SetChar(str, index, c) do {          \
    (str)->chars[(index)] = (c);                        \
}
#define ATX_String_GetLength(str) ((str)->chars?(ATX_String_GetBuffer(str)->length):0)
#define ATX_String_GetChars(str) ((str)->chars?(const char*)((str)->chars):ATX_String_EmptyString)
#define ATX_String_UseChars(str) ((str)->chars?(str)->chars:(char*)ATX_String_EmptyString)
#define ATX_CSTR(str) ATX_String_GetChars(&(str))
#define ATX_String_IsEmpty(str) (ATX_String_GetLength((str))==0)
#define ATX_INIT_STRING(s) do {(s).chars = NULL; } while(0)

/*----------------------------------------------------------------------
|   ATX_String functions
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern ATX_String 
ATX_String_Create(const char* s);

extern ATX_String 
ATX_String_CreateFromSubString(const char* s, ATX_Ordinal first, ATX_Size length);

extern ATX_String 
ATX_String_Clone(const ATX_String* str);

extern void
ATX_String_Copy(ATX_String* str, const ATX_String* other);

extern ATX_Result 
ATX_String_SetLength(ATX_String* str, ATX_Size length);

extern ATX_Result 
ATX_String_Assign(ATX_String* str, const char* chars);

extern ATX_Result 
ATX_String_AssignN(ATX_String* str, const char* chars, ATX_Size size);

extern ATX_Result 
ATX_String_Append(ATX_String* str, const char* other);

extern ATX_Result 
ATX_String_AppendSubString(ATX_String* str, const char* other, ATX_Size size);

extern ATX_Result 
ATX_String_AppendChar(ATX_String* str, char c);

extern ATX_String
ATX_String_Add(const ATX_String* str1, const char* str2);

extern int 
ATX_String_Compare(const ATX_String* str, const char* s, ATX_Boolean ignore_case);

extern ATX_Boolean 
ATX_String_Equals(const ATX_String* str, const char* s, ATX_Boolean ignore_case);

extern ATX_String 
ATX_String_SubString(const ATX_String* str, ATX_Ordinal first, ATX_Size length);

extern ATX_String 
ATX_String_Left(const ATX_String* str, ATX_Size length);

extern ATX_String 
ATX_String_Right(const ATX_String* str, ATX_Size length);

extern ATX_Result
ATX_String_Reserve(ATX_String* str, ATX_Size length);

extern ATX_String
ATX_String_ToLowercase(const ATX_String* str);

extern ATX_String
ATX_String_ToUppercase(const ATX_String* str);

extern ATX_Result
ATX_String_ToInteger(const ATX_String* str, int* value, ATX_Boolean relaxed);

extern ATX_Result
ATX_String_ToFloat(const ATX_String* str, float* value, ATX_Boolean relaxed);

extern void
ATX_String_MakeLowercase(ATX_String* str);

extern void
ATX_String_MakeUppercase(ATX_String* str);

extern void
ATX_String_Replace(ATX_String* str, char a, char b);

extern int  
ATX_String_FindChar(const ATX_String* str, char c);

extern int  
ATX_String_FindCharFrom(const ATX_String* str, char c, ATX_Ordinal start);

extern int  
ATX_String_FindString(const ATX_String* str, const char* s);

extern int  
ATX_String_FindStringFrom(const ATX_String* str, const char* s, ATX_Ordinal start);

extern int  
ATX_String_ReverseFindChar(const ATX_String* str, char c);

extern int  
ATX_String_ReverseFindCharFrom(const ATX_String* str, char c, ATX_Ordinal start);

extern int  
ATX_String_ReverseFindString(const ATX_String* str, const char* s);

extern int  
ATX_String_ReverseFindStringFrom(const ATX_String* str, const char* s, ATX_Ordinal start);

extern ATX_Boolean
ATX_String_StartsWith(const ATX_String* str, const char* s);

extern ATX_Boolean 
ATX_String_EndsWith(const ATX_String* str, const char* s);

extern void
ATX_String_TrimWhitespaceLeft(ATX_String* str);

extern void
ATX_String_TrimCharLeft(ATX_String* str, char c);

extern void
ATX_String_TrimCharsLeft(ATX_String* str, const char* chars);

extern void
ATX_String_TrimWhitespaceRight(ATX_String* str);

extern void
ATX_String_TrimCharRight(ATX_String* str, char c);

extern void
ATX_String_TrimCharsRight(ATX_String* str, const char* chars);

extern void
ATX_String_TrimWhitespace(ATX_String* str);

extern void
ATX_String_TrimChar(ATX_String* str, char c);

extern void
ATX_String_TrimChars(ATX_String* str, const char* chars);

extern ATX_Result
ATX_String_Insert(ATX_String* str, const char* s, ATX_Ordinal where);

/*void Erase(ATX_Ordinal start, ATX_Cardinal count = 1);*/
/*void Replace(ATX_Ordinal start, ATX_Cardinal count, const char* s);*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ATX_STRINGS_H_ */

