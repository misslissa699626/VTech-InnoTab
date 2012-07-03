/*****************************************************************
|
|      Atomix - Helpers: SymbianOS Implementation
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
#include "e32cmn.h"

#include "AtxSymbianUtils.h"
#include "AtxResults.h"
#include "AtxTypes.h"
#include "AtxString.h"
#include "AtxUtils.h"


/*----------------------------------------------------------------------
|       ATX_Symbian_C_PathToUtf16
+---------------------------------------------------------------------*/
ATX_Result
ATX_Symbian_C_PathToUtf16(const char* filename, RBuf& symbian_string)
{
    TUint len = strlen(filename);
    TText* text_name = new TText [len];
    for (unsigned int i=0; i < len; ++i) {
        if (filename[i] == '/')
            text_name[i] = '\\';
        else
            text_name[i] = filename[i];
    }

    symbian_string.Assign(text_name, len, len); // RBuf takes ownership of text_name

    return ATX_SUCCESS;
}


/*----------------------------------------------------------------------
|       ATX_Symbian_CompareToCString
+---------------------------------------------------------------------*/
ATX_Int32
ATX_Symbian_CompareToCString(const TDesC& symbian_str, const char* cstring)
{
    const int sym_len = symbian_str.Length();
    const int c_len = ATX_StringLength(cstring);
    if (sym_len != c_len) {
        return sym_len - c_len;
    }

    for (int i = 0; i < sym_len; ++i) {
        if (symbian_str[i] > cstring[i]) {
            return 1;
        }
        else if (symbian_str[i] < cstring[i]) {
            return -1;
        }
    }

    return 0;
}


/*----------------------------------------------------------------------
|       ATX_Symbian_CStringToSymbian
+---------------------------------------------------------------------*/
ATX_Result
ATX_Symbian_CStringToSymbian(const char* cstring, RBuf& symb_str)
{
    int len = ATX_StringLength(cstring);
    TText* mystr = new TText [len];

    for (int i = 0; i < len; ++i) {
        mystr[i] = cstring[i];
    }

    symb_str.Close();
    symb_str.Assign(mystr, len, len); // RBuf takes ownership of text_name

    return ATX_SUCCESS;
}


/*----------------------------------------------------------------------
|       ATX_String_Assign_FromSymbianString
+---------------------------------------------------------------------*/
ATX_Result
ATX_String_Assign_FromSymbianString(ATX_String* str, const TDesC& symbian_str)
{
    /* Clear the string */
    ATX_String_Assign(str, "");

    ATX_String_Reserve(str, symbian_str.Length() + 1);
    char* buf = ATX_String_UseChars(str);

    for (int i = 0; i < symbian_str.Length(); ++i) {
        TUint16 val = symbian_str[i];
        if (val & 0xff00) {
            /* String contents are actually 16bit, we can't convert that. */
            return ATX_FAILURE;
        }
        buf[i] = (TUint8) val;
    }

    buf[symbian_str.Length()] = '\0';
    return ATX_SUCCESS;
}


