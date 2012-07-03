/*****************************************************************
|
|   BlueTune - Media Utilities
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BlueTune Media Utilities
 */

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltTypes.h"
#include "BltDefs.h"
#include "BltErrors.h"
#include "BltCore.h"
#include "BltMedia.h"

/*----------------------------------------------------------------------
|   global constants
+---------------------------------------------------------------------*/
const BLT_MediaType BLT_MediaType_None = {
    BLT_MEDIA_TYPE_ID_NONE, /* id             */
    0,                      /* flags          */
    0                       /* extension size */
};

const BLT_MediaType BLT_MediaType_Unknown = {
    BLT_MEDIA_TYPE_ID_UNKNOWN, /* id             */
    0,                         /* flags          */
    0                          /* extension size */
};

/*----------------------------------------------------------------------
|   BLT_MediaType_Init
+---------------------------------------------------------------------*/
BLT_Result 
BLT_MediaType_Init(BLT_MediaType* type, BLT_MediaTypeId id)
{
    type->id             = id;
    type->flags          = 0;
    type->extension_size = 0;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   BLT_MediaType_InitEx
+---------------------------------------------------------------------*/
BLT_Result 
BLT_MediaType_InitEx(BLT_MediaType* type, BLT_MediaTypeId id, BLT_Size struct_size)
{
    type->id    = id;
    type->flags = 0;
    if (struct_size < sizeof(BLT_MediaType)) {
        type->extension_size = 0;
        return BLT_ERROR_INVALID_PARAMETERS;
    } else {
        type->extension_size = struct_size-sizeof(BLT_MediaType);
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   BLT_MediaType_Clone
+---------------------------------------------------------------------*/
BLT_Result 
BLT_MediaType_Clone(const BLT_MediaType* from, BLT_MediaType** to)
{
    if (from == NULL) {
    	*to = NULL;
    	return BLT_SUCCESS;
    } else {
	    BLT_Size size = sizeof(BLT_MediaType)+from->extension_size;
	    *to = (BLT_MediaType*)ATX_AllocateMemory(size);
	    if (*to == NULL) return BLT_ERROR_OUT_OF_MEMORY;
	    ATX_CopyMemory((void*)*to, (const void*)from, size);
    }
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   BLT_MediaType_Equals
+---------------------------------------------------------------------*/
BLT_Boolean
BLT_MediaType_Equals(const BLT_MediaType* self, const BLT_MediaType* other)
{
    if (self == NULL || other == NULL) return BLT_FALSE;
    if (self->extension_size != other->extension_size) return BLT_FALSE;
    if (ATX_CompareMemory(self, other, sizeof(BLT_MediaType)+self->extension_size)) return BLT_FALSE;
    return BLT_TRUE;
}

/*----------------------------------------------------------------------
|   BLT_MediaType_Free
+---------------------------------------------------------------------*/
BLT_Result 
BLT_MediaType_Free(BLT_MediaType* type)
{
    if (type) ATX_FreeMemory(type);
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   BLT_MimeParser_ListItemDestructor
+---------------------------------------------------------------------*/
static void
BLT_MimeParser_ListItemDestructor(ATX_ListDataDestructor* self, ATX_Any data, ATX_UInt32 type)
{
    BLT_MimeTypeParameter* parameter = (BLT_MimeTypeParameter*)data;
    ATX_COMPILER_UNUSED(self);
    ATX_COMPILER_UNUSED(type);
    ATX_String_Destruct(&parameter->name);
    ATX_String_Destruct(&parameter->value);
}

/*----------------------------------------------------------------------
|   BLT_ParseMimeType
+---------------------------------------------------------------------*/
/*
Content-Type := type "/" subtype *[";" parameter] 

type :=          "application"     / "audio" 
          / "image"           / "message" 
          / "multipart"  / "text" 
          / "video"           / x-token 

x-token := <The two characters "X-" followed, with no 
           intervening white space, by any token> 

subtype := token 

parameter := attribute "=" value 

attribute := token 

value := token / quoted-string 

token := 1*<any CHAR except SPACE, CTLs, or tspecials> 

tspecials :=  "(" / ")" / "<" / ">" / "@"  ; Must be in 
           /  "," / ";" / ":" / "\" / <">  ; quoted-string, 
           /  "/" / "[" / "]" / "?" / "."  ; to use within 
           /  "="                        ; parameter values
*/
#define BLT_MIME_CHAR_IS_TOKEN(c)            \
    (c>0x20 && c<0x7F &&                     \
     c!='(' && c!=')' && c!='<' && c!='>' && \
     c!='@' && c!=',' && c!=';' && c!=':' && \
     c!='\\'&& c!='"'&& c!='/' && c!='[' &&  \
     c!=']' && c!='?' && c!='.' && c!='=')
     
BLT_Result 
BLT_ParseMimeType(const char* mime_type, ATX_String* main_type, ATX_List** parameters)
{
    int         sem_sep;
    const char* cursor;
    ATX_Result  result = ATX_SUCCESS;
    
    /* parse the main type */
    *parameters = NULL;
    ATX_String_Assign(main_type, mime_type);
    sem_sep = ATX_String_FindChar(main_type, ';');
    if (sem_sep >= 0) {
        ATX_String_SetLength(main_type, sem_sep);
    } else {
        /* no parameters */
        goto end;
    }
        
    /* create a list for the parameters */
    {
        ATX_ListDataDestructor destructor = { NULL, BLT_MimeParser_ListItemDestructor };
        ATX_List_CreateEx(&destructor, parameters);
    }
    
    /* parse all parameters */
    cursor = mime_type+sem_sep;
    while (*cursor) {
        BLT_MimeTypeParameter* parameter = NULL;
        const char* name;
        const char* value = NULL;
        
        /* skip whitespace */
        while (*cursor == ' ' || *cursor == '\t') ++cursor;
        
        /* next is the ';' separator */
        if (*cursor++ != ';') break;
        while (*cursor == ' ' || *cursor == '\t') ++cursor;
        name = cursor;
        
        /* parse until '=' */
        while (*cursor != '\0' && *cursor != '=') ++cursor;
        if (*cursor++ != '=') break;
        
        /* create a new parameter and add it to the list */
        parameter = ATX_AllocateZeroMemory(sizeof(BLT_MimeTypeParameter));
        ATX_String_AssignN(&parameter->name, name, (ATX_Size)(cursor-name-1));
        ATX_String_TrimWhitespace(&parameter->name);
        ATX_List_AddData(*parameters, parameter);
        
        /* check if this is a quoted string or a token */
        if (*cursor == '"') {
            /* quoted string */
            ATX_Boolean in_escape = ATX_FALSE;
            ++cursor;
            while (*cursor) {
                if (in_escape) {
                    in_escape = ATX_FALSE;
                    ++cursor;
                    continue;
                } else if (*cursor == '\\') {
                    in_escape = ATX_TRUE;
                    ++cursor;
                    continue;
                } else if (*cursor == '"') {
                    ++cursor;
                    break;
                }
                ATX_String_AppendChar(&parameter->value, *cursor++);
            }
        } else {
            /* token */
            value = cursor;
            while (BLT_MIME_CHAR_IS_TOKEN(*cursor)) {
                ++cursor;
            } 
            ATX_String_AssignN(&parameter->value, value, (ATX_Size)(cursor-value));
        }
    }
    
end:
    if (ATX_FAILED(result)) {
        if (*parameters) {
            ATX_List_Destroy(*parameters);
            *parameters = NULL;
        }
    }
    ATX_String_TrimWhitespace(main_type);
    
    return result;
}

