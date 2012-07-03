/*****************************************************************
|
|      Atomix Tests - Misc
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
        if (ATX_FAILED(r)) {                                \
            ATX_Debug("failed line %d (%d)\n", __LINE__, r);\
            exit(1);                                        \
        }                                                   \
    } while(0)                                         

#define SHOULD_FAIL(r)                                                  \
    do {                                                                \
        if (ATX_SUCCEEDED(r)) {                                         \
            ATX_Debug("should have failed line %d (%d)\n", __LINE__, r);\
            exit(1);                                                    \
        }                                                               \
    } while(0)                                  

#define SHOULD_EQUAL_I(a, b)                                           \
    do {                                                               \
        if ((a) != (b)) {                                              \
            ATX_Debug("got %l, expected %l line %d\n", a, b, __LINE__);\
            exit(1);                                                   \
        }                                                              \
    } while(0)                                  

#define SHOULD_EQUAL_F(a, b)                                           \
    do {                                                               \
        if ((a) != (b)) {                                              \
            ATX_Debug("got %f, expected %f line %d\n", a, b, __LINE__);\
            exit(1);                                                   \
        }                                                              \
    } while(0)                                  

#define SHOULD_EQUAL_S(a, b)                                           \
    do {                                                               \
        if (!ATX_StringsEqual(a,b)) {                                  \
            ATX_Debug("got %s, expected %s line %d\n", a, b, __LINE__);\
            exit(1);                                                   \
        }                                                              \
    } while(0)                                  

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int 
main(int argc, char** argv)
{
    float      f;
    int        i;
    ATX_Int32  i32;
    ATX_UInt32 ui32;
    ATX_UInt64 ui64;
    char       buff[64];

    ATX_COMPILER_UNUSED(argc);
    ATX_COMPILER_UNUSED(argv);

    SHOULD_FAIL(ATX_ParseInteger("ssdfsdf", &i, ATX_FALSE));
    SHOULD_FAIL(ATX_ParseInteger("", &i, ATX_FALSE));
    SHOULD_FAIL(ATX_ParseInteger(NULL, &i, ATX_FALSE));
    SHOULD_FAIL(ATX_ParseInteger("123a", &i, ATX_FALSE));
    SHOULD_FAIL(ATX_ParseInteger("a123", &i, ATX_FALSE));
    SHOULD_FAIL(ATX_ParseInteger(" 123", &i, ATX_FALSE));
    SHOULD_FAIL(ATX_ParseInteger("a 123", &i, ATX_TRUE));
    SHOULD_FAIL(ATX_ParseInteger(" a123", &i, ATX_TRUE));

    SHOULD_SUCCEED(ATX_ParseInteger("+1", &i, ATX_FALSE));
    SHOULD_EQUAL_I(i, 1);
    SHOULD_SUCCEED(ATX_ParseInteger("+123", &i, ATX_FALSE));
    SHOULD_EQUAL_I(i, 123);
    SHOULD_SUCCEED(ATX_ParseInteger("-1", &i, ATX_FALSE));
    SHOULD_EQUAL_I(i, -1);
    SHOULD_SUCCEED(ATX_ParseInteger("-123", &i, ATX_FALSE));
    SHOULD_EQUAL_I(i, -123);
    SHOULD_SUCCEED(ATX_ParseInteger("-123fgs", &i, ATX_TRUE));
    SHOULD_EQUAL_I(i, -123);
    SHOULD_SUCCEED(ATX_ParseInteger("  -123fgs", &i, ATX_TRUE));
    SHOULD_EQUAL_I(i, -123);
    SHOULD_SUCCEED(ATX_ParseInteger("0", &i, ATX_TRUE));
    SHOULD_EQUAL_I(i, 0);
    SHOULD_SUCCEED(ATX_ParseInteger("7768", &i, ATX_TRUE));
    SHOULD_EQUAL_I(i, 7768);

    SHOULD_SUCCEED(ATX_ParseInteger32("2147483647", &i32, ATX_FALSE));
    SHOULD_EQUAL_I(i32, 2147483647);
    SHOULD_SUCCEED(ATX_ParseInteger32("-2147483647", &i32, ATX_FALSE));
    SHOULD_EQUAL_I(i32, -2147483647);
    SHOULD_SUCCEED(ATX_ParseInteger32("-2147483648", &i32, ATX_FALSE));
    SHOULD_EQUAL_I(i32, (-2147483647 - 1));
    SHOULD_FAIL(ATX_ParseInteger32("2147483648", &i32, ATX_FALSE));
    SHOULD_FAIL(ATX_ParseInteger32("-2147483649", &i32, ATX_FALSE));
    SHOULD_FAIL(ATX_ParseInteger32("-21474836480", &i32, ATX_FALSE));
    SHOULD_FAIL(ATX_ParseInteger32("21474836470", &i32, ATX_FALSE));

    SHOULD_SUCCEED(ATX_ParseInteger32U("4294967295", &ui32, ATX_FALSE));
    SHOULD_EQUAL_I(ui32, 4294967295U);
    SHOULD_FAIL(ATX_ParseInteger32U("4294967296", &ui32, ATX_FALSE));
    SHOULD_FAIL(ATX_ParseInteger32U("-1", &ui32, ATX_FALSE));

    SHOULD_SUCCEED(ATX_ParseInteger64U("32410071309164530", &ui64, ATX_FALSE));

    SHOULD_SUCCEED(ATX_IntegerToString(-123, buff, sizeof(buff)));
    SHOULD_EQUAL_S(buff, "-123");
    SHOULD_FAIL(ATX_IntegerToString(-1234567, buff, 8));
    SHOULD_SUCCEED(ATX_IntegerToString(-1234567, buff, 9));
    SHOULD_EQUAL_S(buff, "-1234567");

    SHOULD_SUCCEED(ATX_IntegerToStringU(123, buff, sizeof(buff)));
    SHOULD_EQUAL_S(buff, "123");
    SHOULD_FAIL(ATX_IntegerToStringU(1234567, buff, 7));
    SHOULD_SUCCEED(ATX_IntegerToStringU(1234567, buff, 8));
    SHOULD_EQUAL_S(buff, "1234567");

    SHOULD_FAIL(ATX_ParseFloat("ssdfsdf", &f, ATX_FALSE));
    SHOULD_FAIL(ATX_ParseFloat("", &f, ATX_FALSE));
    SHOULD_FAIL(ATX_ParseFloat(NULL, &f, ATX_FALSE));
    SHOULD_FAIL(ATX_ParseFloat("123.", &f, ATX_FALSE));
    SHOULD_FAIL(ATX_ParseFloat("a123", &f, ATX_FALSE));
    SHOULD_FAIL(ATX_ParseFloat(" 123", &f, ATX_FALSE));
    SHOULD_FAIL(ATX_ParseFloat(" 127.89E5ff", &f, ATX_FALSE));

    SHOULD_SUCCEED(ATX_ParseFloat("+1.0", &f, ATX_FALSE));
    SHOULD_EQUAL_F(f, 1.0f);
    SHOULD_SUCCEED(ATX_ParseFloat("+123", &f, ATX_FALSE));
    SHOULD_EQUAL_F(f, 123.0f);
    SHOULD_SUCCEED(ATX_ParseFloat("-0.1", &f, ATX_FALSE));
    SHOULD_EQUAL_F(f, -0.1f);
    SHOULD_SUCCEED(ATX_ParseFloat("0.23e-13", &f, ATX_FALSE));
    SHOULD_EQUAL_F(f, 0.23e-13f);
    SHOULD_SUCCEED(ATX_ParseFloat(" 127.89E5ff", &f, ATX_TRUE));
    SHOULD_EQUAL_F(f, 127.89E5f);
    SHOULD_SUCCEED(ATX_ParseFloat("+0.3db", &f, ATX_TRUE));
    SHOULD_EQUAL_F(f, 0.3f);
    SHOULD_SUCCEED(ATX_ParseFloat("+.3db", &f, ATX_TRUE));
    SHOULD_EQUAL_F(f, 0.3f);
    SHOULD_SUCCEED(ATX_ParseFloat("-.3db", &f, ATX_TRUE));
    SHOULD_EQUAL_F(f, -0.3f);
    SHOULD_SUCCEED(ATX_ParseFloat(".3db", &f, ATX_TRUE));
    SHOULD_EQUAL_F(f, .3f);

    SHOULD_SUCCEED(ATX_FloatToString(0.0f, buff, sizeof(buff)));
    SHOULD_EQUAL_S(buff, "0.0");
    SHOULD_SUCCEED(ATX_FloatToString(1.0f, buff, sizeof(buff)));
    SHOULD_EQUAL_S(buff, "1.0");
    SHOULD_SUCCEED(ATX_FloatToString(-1.0f, buff, sizeof(buff)));
    SHOULD_EQUAL_S(buff, "-1.0");
    SHOULD_SUCCEED(ATX_FloatToString(12345.0f, buff, sizeof(buff)));
    SHOULD_EQUAL_S(buff, "12345.0");
    SHOULD_SUCCEED(ATX_FloatToString(12345.002f, buff, sizeof(buff)));
    SHOULD_EQUAL_S(buff, "12345.001953");
    SHOULD_SUCCEED(ATX_FloatToString(-12345.5000f, buff, sizeof(buff)));
    SHOULD_EQUAL_S(buff, "-12345.5");
    SHOULD_SUCCEED(ATX_FloatToString(1.99999f, buff, sizeof(buff)));
    SHOULD_EQUAL_S(buff, "1.999989");
    SHOULD_SUCCEED(ATX_FloatToString(12345E25f, buff, sizeof(buff)));
    /*SHOULD_EQUAL_S(buff, "123450017309192836793256378368.0");*/

    /* IP Address suff */
    {
        ATX_IpAddress ip;
        SHOULD_FAIL(ATX_IpAddress_Parse(&ip, ""));
        SHOULD_FAIL(ATX_IpAddress_Parse(&ip, "a.b.c.d"));
        SHOULD_FAIL(ATX_IpAddress_Parse(&ip, "1.2.3.4.5"));
        SHOULD_FAIL(ATX_IpAddress_Parse(&ip, "1"));
        SHOULD_FAIL(ATX_IpAddress_Parse(&ip, "1.2.3.4."));
        SHOULD_FAIL(ATX_IpAddress_Parse(&ip, "1.2.3.4f"));
        SHOULD_FAIL(ATX_IpAddress_Parse(&ip, "1.g.3.4"));
        SHOULD_FAIL(ATX_IpAddress_Parse(&ip, "1.2..3.4"));
        SHOULD_FAIL(ATX_IpAddress_Parse(&ip, "1.2.300.4"));
        SHOULD_SUCCEED(ATX_IpAddress_Parse(&ip, "1.2.3.4"));
        SHOULD_EQUAL_I(ip[0],1);
        SHOULD_EQUAL_I(ip[1],2);
        SHOULD_EQUAL_I(ip[2],3);
        SHOULD_EQUAL_I(ip[3],4);
        SHOULD_SUCCEED(ATX_IpAddress_Parse(&ip, "255.255.0.1"));
        SHOULD_EQUAL_I(ip[0],255);
        SHOULD_EQUAL_I(ip[1],255);
        SHOULD_EQUAL_I(ip[2],0);
        SHOULD_EQUAL_I(ip[3],1);
        SHOULD_SUCCEED(ATX_IpAddress_Parse(&ip, "0.0.0.0"));
        SHOULD_EQUAL_I(ip[0],0);
        SHOULD_EQUAL_I(ip[1],0);
        SHOULD_EQUAL_I(ip[2],0);
        SHOULD_EQUAL_I(ip[3],0);
    }

    return 0;
}

