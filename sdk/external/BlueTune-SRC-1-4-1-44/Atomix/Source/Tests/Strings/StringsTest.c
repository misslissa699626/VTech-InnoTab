/*****************************************************************
|
|      Stings Test Program
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "AtxString.h"
#include "AtxDebug.h"
#include "AtxUtils.h"

/*----------------------------------------------------------------------
|       Fail
+---------------------------------------------------------------------*/
static void
Fail()
{
    printf("##################################\n");
    exit(1);
}

/*----------------------------------------------------------------------
|       CompareTest
+---------------------------------------------------------------------*/
static void
CompareTest(const char* name, const char* a, const char* b, int result, int expected)
{
    if (result < 0) result = -1;
    if (result > 0) result = 1;
    printf("%s %s %s = %d [%s]\n", a, name, b, result, result == expected ? "pass" : "fail");
    if (result != expected) Fail();
}

/*----------------------------------------------------------------------
|       EqualTest
+---------------------------------------------------------------------*/
static void
EqualTest(const char* name, const char* a, ATX_String b, const char* expected)
{
    printf("op %s on %s, result = %s ", name, a, ATX_CSTR(b));
    if (strcmp(expected, ATX_CSTR(b))) {
        printf(" [fail: exptected %s, got %s]\n", expected, ATX_CSTR(b));
    } else {
        printf(" [pass]\n");
    }
    if (strcmp(expected, ATX_CSTR(b))) Fail();
}

/*----------------------------------------------------------------------
|       StringTest
+---------------------------------------------------------------------*/
static void
StringTest(const char* name, ATX_String a, const char* expected)
{
    printf("%s: %s", name, ATX_CSTR(a));
    if (strcmp(expected, ATX_CSTR(a))) {
        printf(" [fail: exptected %s, got %s]\n", expected, ATX_CSTR(a));
    } else {
        printf(" [pass]\n");
    }
    if (strcmp(expected, ATX_CSTR(a))) Fail();
}

/*----------------------------------------------------------------------
|       IntTest
+---------------------------------------------------------------------*/
static void
IntTest(const char* name, int a, int expected)
{
    printf("%s: %d", name, a);
    if (a != expected) {
        printf(" [fail: exptected %d, got %d]\n", expected, a);
    } else {
        printf(" [pass]\n");
    }
    if (a != expected) Fail();
}

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    ATX_COMPILER_UNUSED(argc);
    ATX_COMPILER_UNUSED(argv);

    printf(":: testing empty string\n");

    {
        ATX_String s = ATX_EMPTY_STRING;
        ATX_ASSERT(sizeof(ATX_String) == sizeof(void*));
        ATX_ASSERT(ATX_String_GetChars(&s)[0] == '\0');
        ATX_String_Destruct(&s);
    }

    printf(":: testing construction and destruction\n");
    {
        ATX_String n0 = ATX_String_Create("Hello");
        ATX_String_Destruct(&n0);
    }
    {
        ATX_String n1 = ATX_String_Create("Bye");
        ATX_String_Assign(&n1, "ByeBye");
        ATX_String_Destruct(&n1);
    }

    printf(":: testing constructors\n");
    {
        ATX_String s00 = ATX_EMPTY_STRING;
        StringTest("constructor()", s00, "");
        ATX_String_Destruct(&s00);
    }
    {
        ATX_String s01 = ATX_String_Create("abcdef");
        StringTest("constructor(const char*)", s01, "abcdef");
        ATX_String_Destruct(&s01);
    }

    {
        ATX_String s03 = ATX_String_CreateFromSubString("abcdefgh", 0, 3);
        StringTest("constructor(const char* s, unsigned int)", s03, "abc");
        ATX_String_Destruct(&s03);
    }

    {
        ATX_String s06 = ATX_String_Create((const char*)NULL);
        StringTest("constructor(NULL)", s06, "");
        ATX_String_Destruct(&s06);
    }

    {
        ATX_String s08 = ATX_String_Create("");
        StringTest("constructor(const char* = \"\")", s08, "");
        ATX_String_Destruct(&s08);
    }

    {
        ATX_String s09 = ATX_String_CreateFromSubString("jkhlkjh\0fgsdfg\0fgsdfg", 0, 10);
        StringTest("ATX_String s09(\"jkhlkjh\0fgsdfg\0fgsdfg\", 0, 10)", s09, "jkhlkjh");
        ATX_String_Destruct(&s09);
    }

    printf(":: testing assignments\n");
    {
        ATX_String a00 = ATX_EMPTY_STRING;
        ATX_String a01 = ATX_EMPTY_STRING;
        ATX_String_Copy(&a01, &a00);
        StringTest("operator=(const ATX_String& = empty)", a01, "");
        ATX_String_Destruct(&a00);
        ATX_String_Destruct(&a01);
    }

    printf(":: testing GetLength\n");
    {
        ATX_String gl1;
        ATX_String gl0 = ATX_String_Create("abcefg");
        IntTest("GetLength", ATX_String_GetLength(&gl0), 6);
        ATX_String_Assign(&gl0, "");
        IntTest("GetLength", ATX_String_GetLength(&gl0), 0);
        ATX_String_Assign(&gl0, "abcd");
        gl1 = ATX_String_Clone(&gl0);
        IntTest("GetLength", ATX_String_GetLength(&gl1), 4);
        ATX_String_Destruct(&gl0);
        ATX_String_Destruct(&gl1);
    }

    printf("::testing Append\n");
    {
        ATX_String l = ATX_String_Create("blabla");
        ATX_String_AppendSubString(&l, "blibliblo", 6);
        StringTest("append(const char*, int size)", l, "blablablibli");
        ATX_String_Destruct(&l);
    }
    {
        ATX_String a = ATX_EMPTY_STRING;
        ATX_String_AppendSubString(&a, "bloblo", 3);
        StringTest("append to NULL", a, "blo");
        ATX_String_Destruct(&a);
    }
    {
        ATX_String a = ATX_String_Create("abc");
        ATX_String_Append(&a, "def");
        StringTest("append 'abc' to 'def'", a, "abcdef");
        ATX_String_Destruct(&a);
    }

    printf("::testing Reserve\n");
    {
        ATX_String r = ATX_String_Create("123");
        ATX_String r_save;
        ATX_String_Reserve(&r, 100);
        r_save = r;
        IntTest("size of string not changed", ATX_String_GetLength(&r), 3);
        ATX_String_Append(&r, "4");
        ATX_String_Append(&r, "5");
        ATX_String_Append(&r, "6");
        ATX_ASSERT(r.chars == r_save.chars);
        ATX_String_Reserve(&r, 0);
        ATX_ASSERT(r.chars == r_save.chars);
        ATX_String_Destruct(&r);
    }

    printf(":: testing substring");
    {
        ATX_String sup = ATX_String_Create("abcdefghijklmnopqrstub");
        ATX_String sub = ATX_String_SubString(&sup, 0, 2);
        StringTest("substring [0,2] of 'abcdefghijklmnopqrstub'", sub, "ab");
        ATX_String_Destruct(&sub);
        sub = ATX_String_SubString(&sup, 3, 4);
        StringTest("substring [3,4] of 'abcdefghijklmnopqrstub'", sub, "defg");
        ATX_String_Destruct(&sub);
        sub = ATX_String_SubString(&sup, 100, 5);
        StringTest("substring [100,5] of 'abcdefghijklmnopqrstub'", sub, "");
        ATX_String_Destruct(&sub);
        sub = ATX_String_SubString(&sup, 8,100);
        StringTest("substring [8,100] of 'abcdefghijklmnopqrstub'", sub, "ijklmnopqrstub");
        ATX_String_Destruct(&sub);
        ATX_String_Destruct(&sup);
    }

    printf(":: testing trims");
    {
        ATX_String trim = ATX_String_Create("*&##just this$&**");
        ATX_String_TrimCharLeft(&trim, '*');
        StringTest("TrimLeft('*') of '*&##just this$&**'", trim, "&##just this$&**");
        ATX_String_TrimCharsLeft(&trim, "*&##");
        StringTest("TrimLeft('&*##')", trim, "just this$&**");
        ATX_String_TrimCharRight(&trim, '*');
        StringTest("TrimRight('*')", trim, "just this$&");
        ATX_String_TrimCharsRight(&trim, "*&##");
        StringTest("TrimRight('*&##')", trim, "just this$");
        ATX_String_Assign(&trim, "*&##just this$&**");
        ATX_String_TrimChars(&trim, "$&*#");
        StringTest("Trim('$&*#') of '*&##just this$&**'", trim, "just this");
        ATX_String_Assign(&trim, "\r\njust this\t   \r\n");
        ATX_String_TrimWhitespace(&trim);
        StringTest("Trim() of '\\r\\njust this\\t   \\r\\n'", trim, "just this");
        ATX_String_Destruct(&trim);
    }

    printf(":: testing Append\n");
    {
        ATX_String o1 = ATX_String_Create("hello");
        ATX_String o2 = ATX_String_Create(", gilles");
        ATX_String_Append(&o1,  ATX_CSTR(o2));  
        StringTest("Append", o1, "hello, gilles");
        ATX_String_Append(&o1, ", some more");
        StringTest("Append", o1, "hello, gilles, some more");

        printf(":: testing GetChar\n");
        ATX_String_Assign(&o1, "abcdefgh");
        IntTest("o1[0]", 'a', ATX_String_GetChar(&o1, 0));
        IntTest("o1[1]", 'b', ATX_String_GetChar(&o1, 1));
        IntTest("o1[2]", 'c', ATX_String_GetChar(&o1, 2));
        ATX_String_Destruct(&o1);
        ATX_String_Destruct(&o2);
    }

    printf(":: testing CompareNoCase\n");
    {
        ATX_String s = ATX_String_Create("abc");
        CompareTest("cnc", "abc", "abc", ATX_String_Compare(&s, "abc", ATX_TRUE), 0);
        ATX_String_Assign(&s, "AbC3");
        CompareTest("cnc", "AbC3", "aBC3", ATX_String_Compare(&s, "aBC3", ATX_TRUE), 0);
        ATX_String_Assign(&s, "AbCc");
        CompareTest("cnc", "AbC3", "aBC3", ATX_String_Compare(&s, "aBC3", ATX_FALSE), -1);
        ATX_String_Assign(&s, "AbCc");
        CompareTest("cnc", "AbCc", "aBcD", ATX_String_Compare(&s, "aBcD", ATX_TRUE), -1);
        ATX_String_Assign(&s, "AbCC");
        CompareTest("cnc", "AbCC", "aBcd", ATX_String_Compare(&s, "aBcd", ATX_TRUE), -1);
        ATX_String_Assign(&s, "bbCc");
        CompareTest("cnc", "bbCc", "aBcc", ATX_String_Compare(&s, "aBcc", ATX_TRUE), 1);
        ATX_String_Assign(&s, "BbCC");
        CompareTest("cnc", "BbCC", "aBcc", ATX_String_Compare(&s, "aBcc", ATX_TRUE), 1);
        ATX_String_Destruct(&s);
    }

    printf(":: testing MakeLowercase\n");
    {
        ATX_String lower = ATX_String_Create("abcdEFGhijkl");
        ATX_String t;
        ATX_String_MakeLowercase(&lower);
        EqualTest("MakeLowercase (noref)", "abcdEFGhijkl", lower, "abcdefghijkl");

        printf(":: testing ToLowercase\n");
        ATX_String_Assign(&lower, "abcdEFGhijkl");
        t = ATX_String_ToLowercase(&lower);
        EqualTest("ToLowercase", "abcdEFGhijkl", t, "abcdefghijkl");
        ATX_String_Destruct(&lower);
        ATX_String_Destruct(&t);
    }

    printf(":: testing MakeUppercase\n");
    {
        ATX_String upper = ATX_String_Create("abcdEFGhijkl");
        ATX_String t;
        ATX_String_MakeUppercase(&upper);
        EqualTest("MakeUppercase (noref)", "abcdEFGhijkl", upper, "ABCDEFGHIJKL");

        printf(":: testing ToUppercase\n");
        ATX_String_Assign(&upper, "abcdEFGhijkl");
        t = ATX_String_ToUppercase(&upper);
        EqualTest("ToUppercase", "abcdEFGhijkl", t, "ABCDEFGHIJKL");
        ATX_String_Destruct(&upper);
        ATX_String_Destruct(&t);
    }

    printf(":: testing Find (s=\"au clair de la lune\")\n");
    {
        ATX_String s = ATX_String_Create("au clair de la lune");
        ATX_String s1 = ATX_EMPTY_STRING;
        int f = ATX_String_FindString(&s, "au");
        IntTest("Find(\"au\")", f, 0);
        f = ATX_String_FindString(&s, "clair");
        IntTest("Find(\"clair\")", f, 3);
        f = ATX_String_FindString(&s, "luneb");
        IntTest("Find(\"luneb\")", f, -1);
        f = ATX_String_FindString(&s, (const char*)NULL);
        IntTest("Find(NULL)", f, -1);
        f = ATX_String_FindString(&s, "hello");
        IntTest("Find(\"hello\")", f, -1);
        f = ATX_String_FindString(&s, "");
        IntTest("Find(\"\")", f, 0);
        f = ATX_String_FindStringFrom(&s, "clair", 2);
        IntTest("Find(\"clair\", 2)", f, 3);
        f = ATX_String_FindStringFrom(&s, "clair", 100);
        IntTest("Find(\"clair\", 100)", f, -1);
        f = ATX_String_FindString(&s, "au clair de la lune");
        IntTest("Find(\"au clair de la lune\")", f, 0);
        f = ATX_String_FindString(&s, "au clair de la lune mon ami");
        IntTest("Find(\"au clair de la lune mon ami\")", f, -1);
        f = ATX_String_FindChar(&s, 'c');
        IntTest("Find('c')", f, 3);
        f = ATX_String_FindString(&s1, "hello");
        IntTest("Find() in empty string", f, -1);
        ATX_String_Destruct(&s);
        ATX_String_Destruct(&s1);
    }

    printf(":: testing Replace\n");
    {
        ATX_String r0 = ATX_String_Create("abcdefghijefe");
        ATX_String_Replace(&r0, 'e','@');
        StringTest("Replace(char, char)", r0, "abcd@fghij@f@");
        ATX_String_Destruct(&r0);
    }

    printf(":: testing Insert\n");
    {
        ATX_String in0 = ATX_EMPTY_STRING;
        ATX_String_Insert(&in0, "hello", 1);
        StringTest("Insert into NULL, past end", in0, "");
        ATX_String_Insert(&in0, "hello", 0);
        StringTest("Insert into NULL, at start", in0, "hello");
        ATX_String_Insert(&in0, "yoyo", 0);
        StringTest("Insert at start", in0, "yoyohello");
        ATX_String_Insert(&in0, "yaya", 3);
        StringTest("Insert at 3", in0, "yoyyayaohello");
        ATX_String_Destruct(&in0);
    }

    return 0;
}
