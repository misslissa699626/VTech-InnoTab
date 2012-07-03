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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "AtxConfig.h"
#include "AtxTypes.h"
#include "AtxString.h"
#include "AtxResults.h"
#include "AtxUtils.h"
#include "AtxDebug.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define ATX_STRINGS_WHITESPACE_CHARS "\r\n\t "

/*----------------------------------------------------------------------
|   helpers
+---------------------------------------------------------------------*/
#define ATX_UPPERCASE(x) (((x) >= 'a' && (x) <= 'z') ? (x)&0xdf : (x))
#define ATX_LOWERCASE(x) (((x) >= 'A' && (x) <= 'Z') ? (x)^32   : (x))
#define ATX_STRING_BUFFER_CHARS(b) ((char*)((b)+1))

/*----------------------------------------------------------------------
|   ATX_String_EmptyString
+---------------------------------------------------------------------*/
const char* const ATX_String_EmptyString = "";

/*----------------------------------------------------------------------
|   ATX_StringBuffer_Allocate
+---------------------------------------------------------------------*/
static ATX_StringBuffer*
ATX_StringBuffer_Allocate(ATX_Size allocated, ATX_Size length) 
{
    ATX_StringBuffer* buffer = 
        (ATX_StringBuffer*)
        ATX_AllocateMemory(sizeof(ATX_StringBuffer)+allocated+1);
    buffer->length = length;
    buffer->allocated = allocated;

    return buffer;
}

/*----------------------------------------------------------------------
|   ATX_StringBuffer_Create
+---------------------------------------------------------------------*/
static char* 
ATX_StringBuffer_Create(ATX_Size length)
{
    /* allocate a buffer of the requested size */
    ATX_StringBuffer* buffer = ATX_StringBuffer_Allocate(length, length);
    return ATX_STRING_BUFFER_CHARS(buffer);
}

/*----------------------------------------------------------------------
|   ATX_StringBuffer_CreateFromString
+---------------------------------------------------------------------*/
static char* 
ATX_StringBuffer_CreateFromString(const char* str)
{
    /* allocate a buffer of the same size as str */
    ATX_Size length = ATX_StringLength(str);
    ATX_StringBuffer* buffer = ATX_StringBuffer_Allocate(length, length);
    char* result = ATX_STRING_BUFFER_CHARS(buffer);
    
    /* copy the string in the new buffer */
    ATX_CopyString(result, str);

    return result;
}

/*----------------------------------------------------------------------
|   ATX_StringBuffer_CreateFromStringN
+---------------------------------------------------------------------*/
static char* 
ATX_StringBuffer_CreateFromStringN(const char* str, ATX_Size length)
{
    /* allocate a buffer of the requested size */
    ATX_StringBuffer* buffer = ATX_StringBuffer_Allocate(length, length);
    char* result = ATX_STRING_BUFFER_CHARS(buffer);

    /* copy the string in the new buffer */
    ATX_CopyMemory(result, str, length);

    /* add a null-terminator */
    result[length] = '\0';

    return result;
}

/*----------------------------------------------------------------------
|   ATX_String_Create
+---------------------------------------------------------------------*/
ATX_String
ATX_String_Create(const char* str)
{
    ATX_String result;
    if (str == NULL || str[0] == '\0') {
        result.chars = NULL;
    } else {
        result.chars = ATX_StringBuffer_CreateFromString(str);
    }

    return result;
}

/*----------------------------------------------------------------------
|   ATX_String_CreateFromSubString
+---------------------------------------------------------------------*/
ATX_String
ATX_String_CreateFromSubString(const char* str,
                               ATX_Ordinal first, 
                               ATX_Size    length)
{
    ATX_String result;

    /* shortcut */
    if (str != NULL && length != 0) {
        /* possibly truncate length */
        ATX_Size str_length = 0;
        const char* src_str = str + first;
        while (*src_str) {
            ++str_length;
            ++src_str;
            if (str_length >= length) break;
        }
        if (str_length != 0) {
            result.chars = ATX_StringBuffer_CreateFromStringN(str+first, str_length);
            return result;
        }
    } 
    result.chars = NULL;

    return result;
}

/*----------------------------------------------------------------------
|   ATX_String_Clone
+---------------------------------------------------------------------*/
ATX_String
ATX_String_Clone(const ATX_String* self)
{
    ATX_String result;
    if (self->chars == NULL) {
        result.chars = NULL;
    } else {
        ATX_Size length = ATX_String_GetLength(self);
        result.chars = ATX_StringBuffer_Create(length);
        ATX_CopyString(result.chars, self->chars);
    }

    return result;
}


/*----------------------------------------------------------------------
|   ATX_String_Reset
+---------------------------------------------------------------------*/
static void
ATX_String_Reset(ATX_String* self)
{
    if (self->chars != NULL) {
        ATX_FreeMemory((void*)ATX_String_GetBuffer(self));
        self->chars = NULL;
    }
}

/*----------------------------------------------------------------------
|   ATX_String_PrepareToWrite
+---------------------------------------------------------------------*/
static char*
ATX_String_PrepareToWrite(ATX_String* self, ATX_Size length)
{
    if (self->chars == NULL || ATX_String_GetBuffer(self)->allocated < length) {
        /* the buffer is too small, we need to allocate a new one */
        ATX_Size needed = length;
        if (self->chars != NULL) {
            ATX_Size grow = ATX_String_GetBuffer(self)->allocated*2;
            if (grow > length) needed = grow;
            ATX_FreeMemory((void*)ATX_String_GetBuffer(self));
        }
        self->chars = ATX_STRING_BUFFER_CHARS(
            ATX_StringBuffer_Allocate(needed, length));
    } else {   
        ATX_String_GetBuffer(self)->length = length;
    }
    return self->chars;
}

/*----------------------------------------------------------------------
|   ATX_String_Reserve
+---------------------------------------------------------------------*/
ATX_Result
ATX_String_Reserve(ATX_String* self, ATX_Size allocate)
{
    if (self->chars == NULL || ATX_String_GetBuffer(self)->allocated < allocate) {
        /* the buffer is too small, we need to allocate a new one */
        ATX_Size needed = allocate;
        if (self->chars != NULL) {
            ATX_Size grow = ATX_String_GetBuffer(self)->allocated*2;
            if (grow > allocate) needed = grow;
        }
        {
            ATX_Size length = ATX_String_GetLength(self);
            char* copy = ATX_STRING_BUFFER_CHARS(
                ATX_StringBuffer_Allocate(needed, length));
            if (copy == NULL) return ATX_ERROR_OUT_OF_MEMORY;
            if (self->chars != NULL) {
                ATX_CopyString(copy, self->chars);
                ATX_FreeMemory(ATX_String_GetBuffer(self));
            } else {
                copy[0] = '\0';
            }
            self->chars = copy;
        }
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_String_Assign
+---------------------------------------------------------------------*/
ATX_Result
ATX_String_Assign(ATX_String* self, const char* str)
{
    if (str == NULL) {
        ATX_String_Reset(self);
        return ATX_SUCCESS;
    } else {
        return ATX_String_AssignN(self, str, ATX_StringLength(str));
    }
}

/*----------------------------------------------------------------------
|   ATX_String_AssignN
+---------------------------------------------------------------------*/
ATX_Result
ATX_String_AssignN(ATX_String* self, const char* str, ATX_Size length)
{
    if (str == NULL || length == 0) {
        ATX_String_Reset(self);
    } else {
        ATX_String_PrepareToWrite(self, length);
        ATX_CopyMemory(self->chars, str, length);
        self->chars[length] = '\0';
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_String_Copy
+---------------------------------------------------------------------*/
void
ATX_String_Copy(ATX_String* self, const ATX_String* str)
{
    if (str == NULL || str->chars == NULL) {
        ATX_String_Reset(self);
    } else {
        ATX_Size length = ATX_String_GetBuffer(str)->length;
        if (length == 0) {
            ATX_String_Reset(self);
        } else {
            ATX_String_PrepareToWrite(self, length);
            ATX_CopyString(self->chars, str->chars);
        }
    }
}

/*----------------------------------------------------------------------
|   ATX_String_SetLength
+---------------------------------------------------------------------*/
ATX_Result
ATX_String_SetLength(ATX_String* self, ATX_Size length)
{
    if (self->chars == NULL) {
        return (length == 0 ? ATX_SUCCESS : ATX_ERROR_INVALID_PARAMETERS);
    }
    if (length <= ATX_String_GetBuffer(self)->allocated) {
        char* chars = ATX_String_UseChars(self);
        ATX_String_GetBuffer(self)->length = length;
        chars[length] = '\0';
        return ATX_SUCCESS;
    } else {
        return ATX_ERROR_INVALID_PARAMETERS;
    }
}

/*----------------------------------------------------------------------
|   ATX_String_Append
+---------------------------------------------------------------------*/
ATX_Result
ATX_String_Append(ATX_String* self, const char* str)
{
    /* shortcut */
    if (str == NULL || str[0] == '\0') return ATX_SUCCESS;
    return ATX_String_AppendSubString(self, str, ATX_StringLength(str));
}

/*----------------------------------------------------------------------
|   ATX_String_AppendChar
+---------------------------------------------------------------------*/
ATX_Result
ATX_String_AppendChar(ATX_String* self, char c)
{
    return ATX_String_AppendSubString(self, &c, 1);
}

/*----------------------------------------------------------------------
|   ATX_String_AppendSubString
+---------------------------------------------------------------------*/
ATX_Result
ATX_String_AppendSubString(ATX_String* self, const char* str, ATX_Size length)
{
    /* shortcut */
    if (str == NULL || length == 0) return ATX_SUCCESS;

    {
        /* compute the new length */
        ATX_Size old_length = ATX_String_GetLength(self);
        ATX_Size new_length = old_length + length;

        /* allocate enough space */
        ATX_CHECK(ATX_String_Reserve(self, new_length));

        /* append the new string at the end of the current one */
        ATX_CopyMemory(self->chars+old_length, str, length);

        /* set the length and null-terminate */
        ATX_String_GetBuffer(self)->length = new_length;
        self->chars[new_length] = '\0';
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_String_Compare
+---------------------------------------------------------------------*/
int 
ATX_String_Compare(const ATX_String* self, const char *s, ATX_Boolean ignore_case)
{
    const char *r1 = ATX_String_GetChars(self);
    const char *r2 = s;

    if (ignore_case) {
        while (ATX_UPPERCASE(*r1) == ATX_UPPERCASE(*r2)) {
            if (*r1++ == '\0') {
                return 0;
            } 
            r2++;
        }
        return ATX_UPPERCASE(*r1) - ATX_UPPERCASE(*r2);
    } else {
        while (*r1 == *r2) {
            if (*r1++ == '\0') {
                return 0;
            } 
            r2++;
        }
        return (*r1 - *r2);
    }
}

/*----------------------------------------------------------------------
|   ATX_String_Equals
+---------------------------------------------------------------------*/
ATX_Boolean
ATX_String_Equals(const ATX_String* self, const char *s, ATX_Boolean ignore_case)
{
    return ATX_String_Compare(self, s, ignore_case) == 0 ? ATX_TRUE : ATX_FALSE;
}

/*----------------------------------------------------------------------
|   ATX_String_SubString
+---------------------------------------------------------------------*/
ATX_String
ATX_String_SubString(const ATX_String* self, ATX_Ordinal first, ATX_Size length)
{
    return ATX_String_CreateFromSubString(ATX_String_GetChars(self), 
                                          first, 
                                          length);
}

/*----------------------------------------------------------------------
|   ATX_StringStartsWith
|
|    returns:
|   1 if str starts with sub,
|   0 if str is large enough but does not start with sub
|     -1 if str is too short to start with sub
+---------------------------------------------------------------------*/
static int
ATX_StringStartsWith(const char* str, const char* sub)
{
    while (*str == *sub) {
        if (*str++ == '\0') {
            return 1;
        }
        sub++;
    }
    return (*sub == '\0') ? 1 : (*str == '\0' ? -1 : 0);
}

/*----------------------------------------------------------------------
|   ATX_String_StartsWith
+---------------------------------------------------------------------*/
ATX_Boolean 
ATX_String_StartsWith(const ATX_String* self, const char *s)
{
    return (ATX_StringStartsWith(ATX_String_GetChars(self), s) == 1)?ATX_TRUE:ATX_FALSE;
}

/*----------------------------------------------------------------------
|   ATX_String_EndsWith
+---------------------------------------------------------------------*/
ATX_Boolean 
ATX_String_EndsWith(const ATX_String* self, const char *s)
{
    ATX_Size str_length;
    if (s == NULL || *s == '\0') return ATX_FALSE;
    str_length = ATX_StringLength(s);
    if (str_length > ATX_String_GetLength(self)) return ATX_FALSE;
    return (ATX_StringStartsWith(self->chars+ATX_String_GetLength(self)-str_length, s) == 1)?ATX_TRUE:ATX_FALSE;
}


/*----------------------------------------------------------------------
|   ATX_String_FindStringFrom
+---------------------------------------------------------------------*/
int
ATX_String_FindStringFrom(const ATX_String* self, 
                          const char*       str, 
                          ATX_Ordinal       start)
{
    /* check args */
    if (str == NULL || start >= ATX_String_GetLength(self)) return -1;

    /* skip to start position */
    {
        const char* src = self->chars + start;

        /* look for a substring */
        while (*src) {
            int cmp = ATX_StringStartsWith(src, str);
            switch (cmp) {
                case -1:
                    /* ref is too short, abort */
                    return -1;
                case 1:
                    /* match */
                    return (int)(src-self->chars);
            }
            src++;
        }
    }

    return -1;
}

/*----------------------------------------------------------------------
|   ATX_String_FindString
+---------------------------------------------------------------------*/
int
ATX_String_FindString(const ATX_String* self, const char* str)
{
    return ATX_String_FindStringFrom(self, str, 0);
}

/*----------------------------------------------------------------------
|   ATX_String_FindCharFrom
+---------------------------------------------------------------------*/
int
ATX_String_FindCharFrom(const ATX_String* self, char c, ATX_Ordinal start)
{
    /* check args */
    if (start >= ATX_String_GetLength(self)) return -1;

    {
        /* skip to start position */
        const char* src = self->chars + start;

        /* look for the character */
        while (*src) {
            if (*src == c) return (int)(src-self->chars);
            src++;
        }
    }

    return -1;
}

/*----------------------------------------------------------------------
|   ATX_String_FindChar
+---------------------------------------------------------------------*/
int
ATX_String_FindChar(const ATX_String* self, char c)
{
    return ATX_String_FindCharFrom(self, c, 0);
}

/*----------------------------------------------------------------------
|   ATX_String_ReverseFindCharFrom
+---------------------------------------------------------------------*/
int
ATX_String_ReverseFindCharFrom(const ATX_String* self, char c, ATX_Ordinal start)
{
    const char* src = ATX_String_GetChars(self);

    /* check args */
    ATX_Size length = ATX_String_GetLength(self);
    int i = length-start-1;
    if (i < 0) return -1;

    /* look for the character */
    for (;i>=0;i--) {
        if (src[i] == c) return i;
    }

    return -1;
}

/*----------------------------------------------------------------------
|   ATX_String_ReverseFindChar
+---------------------------------------------------------------------*/
int
ATX_String_ReverseFindChar(const ATX_String* self, char c)
{
    return ATX_String_ReverseFindCharFrom(self, c, 0);
}

/*----------------------------------------------------------------------
|   ATX_String_ReverseFindString
+---------------------------------------------------------------------*/
int
ATX_String_ReverseFindString(const ATX_String* self, const char* s)
{
    const char* src = ATX_String_GetChars(self);
    
    /* check args */
    ATX_Size my_length = ATX_String_GetLength(self);
    ATX_Size s_length  = ATX_StringLength(s);
    int i = my_length - s_length;
    if (i < 0) return -1;
    
    /* look for the string */
    for (;i>=0;i--) {
        int cmp = ATX_StringStartsWith(src+i, s);
        if (cmp == 1) {
            /* match */
            return i;
        }
    }
    
    return -1;
}

/*----------------------------------------------------------------------
|   ATX_String_MakeLowercase
+---------------------------------------------------------------------*/
void
ATX_String_MakeLowercase(ATX_String* self)
{
    /* the source is the current buffer */
    char* src = ATX_String_UseChars(self);

    /* convert all the characters of the existing buffer */
    char* dst = src;
    while (*dst != '\0') {
        *dst = ATX_LOWERCASE(*dst);
        dst++;
    }
}

/*----------------------------------------------------------------------
|   ATX_String_MakeUppercase
+---------------------------------------------------------------------*/
void
ATX_String_MakeUppercase(ATX_String* self) 
{
    /* the source is the current buffer */
    char* src = ATX_String_UseChars(self);

    /* convert all the characters of the existing buffer */
    char* dst = src;
    while (*dst != '\0') {
        *dst = ATX_UPPERCASE(*dst);
        dst++;
    }
}

/*----------------------------------------------------------------------
|   ATX_String_ToLowercase
+---------------------------------------------------------------------*/
ATX_String
ATX_String_ToLowercase(const ATX_String* self)
{
    ATX_String result = ATX_String_Clone(self);
    ATX_String_MakeLowercase(&result);
    return result;
}

/*----------------------------------------------------------------------
|   ATX_String_ToUppercase
+---------------------------------------------------------------------*/
ATX_String
ATX_String_ToUppercase(const ATX_String* self)
{
    ATX_String result = ATX_String_Clone(self);
    ATX_String_MakeUppercase(&result);
    return result;
}

/*----------------------------------------------------------------------
|   ATX_String_Replace
+---------------------------------------------------------------------*/
void
ATX_String_Replace(ATX_String* self, char a, char b) 
{
    /* check args */
    if (self->chars == NULL || a == '\0' || b == '\0') return;

    {
        /* we are going to modify the characters */
        char* src = self->chars;

        /* process the buffer in place */
        while (*src) {
            if (*src == a) *src = b;
            src++;
        }
    }
}

/*----------------------------------------------------------------------
|   ATX_String_Insert
+---------------------------------------------------------------------*/
ATX_Result
ATX_String_Insert(ATX_String* self, const char* str, ATX_Ordinal where)
{
    ATX_Size str_length;
    ATX_Size old_length;
    ATX_Size new_length;

    /* check args */
    if (str == NULL) return ATX_SUCCESS;
    if (where > ATX_String_GetLength(self)) return ATX_ERROR_INVALID_PARAMETERS;

    /* measure the string to insert */
    str_length = ATX_StringLength(str);
    if (str_length == 0) return ATX_SUCCESS;

    /* compute the size of the new string */
    old_length = ATX_String_GetLength(self);
    new_length = str_length + ATX_String_GetLength(self);

    {
        /* prepare to write the new string */
        char* src = self->chars;
        char* nst = ATX_StringBuffer_Create(new_length);
        char* dst = nst;

        /* check for errors */
        if (nst == NULL) return ATX_ERROR_OUT_OF_MEMORY;

        /* copy the beginning of the old string */
        if (where > 0) {
            ATX_CopyMemory(dst, src, where);
            src += where;
            dst += where;
        }

        /* copy the inserted string */
        ATX_CopyString(dst, str);
        dst += str_length;

        /* copy the end of the old string */
        if (old_length > where) {
            ATX_CopyString(dst, src);
        }

        /* use the new string */
        if (self->chars) ATX_FreeMemory((void*)ATX_String_GetBuffer(self));
        self->chars = nst;
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_String_ToInteger
+---------------------------------------------------------------------*/
ATX_Result 
ATX_String_ToInteger(const ATX_String* self, int* value, ATX_Boolean relaxed)
{
    return ATX_ParseInteger(ATX_String_GetChars(self), value, relaxed);
}

/*----------------------------------------------------------------------
|    ATX_String_ToFloat
+---------------------------------------------------------------------*/
ATX_Result 
ATX_String_ToFloat(const ATX_String* self, float* value, ATX_Boolean relaxed)
{
    return ATX_ParseFloat(ATX_String_GetChars(self), value, relaxed);
}

/*----------------------------------------------------------------------
|   ATX_String_TrimWhitespaceLeft
+---------------------------------------------------------------------*/
void 
ATX_String_TrimWhitespaceLeft(ATX_String* self)
{
    ATX_String_TrimCharsLeft(self, ATX_STRINGS_WHITESPACE_CHARS);
}

/*----------------------------------------------------------------------
|   ATX_String_TrimCharLeft
+---------------------------------------------------------------------*/
void 
ATX_String_TrimCharLeft(ATX_String* self, char c)
{
    char s[2];
    s[0] = c;
    s[1] = 0;
    ATX_String_TrimCharsLeft(self, (const char*)s);
}

/*----------------------------------------------------------------------
|   ATX_String_TrimCharsLeft
+---------------------------------------------------------------------*/
void 
ATX_String_TrimCharsLeft(ATX_String* self, const char* chars)
{
    const char* s;
    char        c;

    if (self->chars == NULL) return;
    s = self->chars;
    while ((c = *s)) {
        const char* x = chars;
        while (*x) {
            if (*x == c) break;
            x++;
        }
        if (*x == 0) break; /* not found */
        s++;
    }
    if (s == self->chars) {
        /* nothing was trimmed */
        return;
    }

    /* shift chars to the left */
    {
        char* d = self->chars;
        ATX_String_GetBuffer(self)->length = ATX_String_GetLength(self)-(s-d);
        while ((*d++ = *s++)) {};
    }
}

/*----------------------------------------------------------------------
|   ATX_String_TrimWhitespaceRight
+---------------------------------------------------------------------*/
void 
ATX_String_TrimWhitespaceRight(ATX_String* self)
{
    ATX_String_TrimCharsRight(self, ATX_STRINGS_WHITESPACE_CHARS);
}

/*----------------------------------------------------------------------
|   ATX_String_TrimCharRight
+---------------------------------------------------------------------*/
void 
ATX_String_TrimCharRight(ATX_String* self, char c)
{
    char s[2];
    s[0] = c;
    s[1] = 0;
    ATX_String_TrimCharsRight(self, (const char*)s);
}

/*----------------------------------------------------------------------
|   ATX_String_TrimCharsRight
+---------------------------------------------------------------------*/
void 
ATX_String_TrimCharsRight(ATX_String* self, const char* chars)
{
    if (self->chars == NULL || self->chars[0] == '\0') return;

    {
        char* tail = self->chars+ATX_String_GetLength(self)-1;
        char* s = tail;
        while (s != self->chars-1) {
            const char* x = chars;
            while (*x) {
                if (*x == *s) {
                    *s = '\0';
                    break;
                }
                x++;
            }
            if (*x == 0) break; /* not found */
            s--;
        }
        if (s == tail) {
            /* nothing was trimmed */
            return;
        }
        ATX_String_GetBuffer(self)->length = 1+(int)(s-self->chars);
    }
}

/*----------------------------------------------------------------------
|   ATX_String_TrimWhitespace
+---------------------------------------------------------------------*/
void 
ATX_String_TrimWhitespace(ATX_String* self)
{
    ATX_String_TrimWhitespaceLeft(self);
    ATX_String_TrimWhitespaceRight(self);
}

/*----------------------------------------------------------------------
|   ATX_String_TrimChar
+---------------------------------------------------------------------*/
void 
ATX_String_TrimChar(ATX_String* self, char c)
{
    char s[2];
    s[0] = c;
    s[1] = 0;
    ATX_String_TrimCharsLeft(self, (const char*)s);
    ATX_String_TrimCharsRight(self, (const char*)s);
}

/*----------------------------------------------------------------------
|   ATX_String_TrimChars
+---------------------------------------------------------------------*/
void 
ATX_String_TrimChars(ATX_String* self, const char* chars)
{
    ATX_String_TrimCharsLeft(self, chars);
    ATX_String_TrimCharsRight(self, chars);
}

/*----------------------------------------------------------------------
|   ATX_String_Add
+---------------------------------------------------------------------*/
ATX_String 
ATX_String_Add(const ATX_String* s1, const char* s2)
{
    /* shortcut */
    if (s2 == NULL || s2[0] == '\0') return ATX_String_Clone(s1);

    {
        /* measure strings */
        ATX_Size s1_length = ATX_String_GetLength(s1);
        ATX_Size s2_length = ATX_StringLength(s2);

        /* allocate space for the new string */
        ATX_String result = ATX_EMPTY_STRING;
        char* start = ATX_String_PrepareToWrite(&result, s1_length+s2_length);

        /* concatenate the two strings into the result */
        ATX_CopyMemory(start, s1, s1_length);
        ATX_CopyString(start+s1_length, s2);

        return result;
    }
}

