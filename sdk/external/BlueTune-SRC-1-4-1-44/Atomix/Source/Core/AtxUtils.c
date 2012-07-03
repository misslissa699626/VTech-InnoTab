/*****************************************************************
|
|   Atomix - Utils
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
#include "AtxUtils.h"
#include "AtxResults.h"
#include "AtxDebug.h"
#if defined(ATX_CONFIG_HAVE_MATH_H)
#include <math.h>
#endif
#if defined(ATX_CONFIG_HAVE_LIMITS_H)
#include <limits.h>
#endif

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define ATX_FORMAT_LOCAL_BUFFER_SIZE 1024
#define ATX_FORMAT_BUFFER_INCREMENT  4096
#define ATX_FORMAT_BUFFER_MAX_SIZE   65536

/*----------------------------------------------------------------------
|    ATX_BytesFromInt64Be
+---------------------------------------------------------------------*/
void 
ATX_BytesFromInt64Be(unsigned char* buffer, ATX_UInt64 value)
{
    buffer[0] = (unsigned char)(value>>56) & 0xFF;
    buffer[1] = (unsigned char)(value>>48) & 0xFF;
    buffer[2] = (unsigned char)(value>>40) & 0xFF;
    buffer[3] = (unsigned char)(value>>32) & 0xFF;
    buffer[4] = (unsigned char)(value>>24) & 0xFF;
    buffer[5] = (unsigned char)(value>>16) & 0xFF;
    buffer[6] = (unsigned char)(value>> 8) & 0xFF;
    buffer[7] = (unsigned char)(value    ) & 0xFF;
}

/*----------------------------------------------------------------------
|    ATX_BytesFromInt32Be
+---------------------------------------------------------------------*/
void 
ATX_BytesFromInt32Be(unsigned char* buffer, ATX_UInt32 value)
{
    buffer[0] = (unsigned char)(value>>24) & 0xFF;
    buffer[1] = (unsigned char)(value>>16) & 0xFF;
    buffer[2] = (unsigned char)(value>> 8) & 0xFF;
    buffer[3] = (unsigned char)(value    ) & 0xFF;
}

/*----------------------------------------------------------------------
|    ATX_BytesFromInt16Be
+---------------------------------------------------------------------*/
void 
ATX_BytesFromInt16Be(unsigned char* buffer, ATX_UInt16 value)
{
    buffer[0] = (unsigned char)((value>> 8) & 0xFF);
    buffer[1] = (unsigned char)((value    ) & 0xFF);
}

/*----------------------------------------------------------------------
|    ATX_BytesToInt32Be
+---------------------------------------------------------------------*/
ATX_UInt32 
ATX_BytesToInt32Be(const unsigned char* buffer)
{
    return 
        ( ((ATX_UInt32)buffer[0])<<24 ) |
        ( ((ATX_UInt32)buffer[1])<<16 ) |
        ( ((ATX_UInt32)buffer[2])<<8  ) |
        ( ((ATX_UInt32)buffer[3])     );
}

/*----------------------------------------------------------------------
|    ATX_BytesToInt64Be
+---------------------------------------------------------------------*/
ATX_UInt64 
ATX_BytesToInt64Be(const unsigned char* buffer)
{
    return 
        ( ((ATX_UInt64)buffer[0])<<56 ) |
        ( ((ATX_UInt64)buffer[1])<<48 ) |
        ( ((ATX_UInt64)buffer[2])<<40 ) |
        ( ((ATX_UInt64)buffer[3])<<32 ) |
        ( ((ATX_UInt64)buffer[4])<<24 ) |
        ( ((ATX_UInt64)buffer[5])<<16 ) |
        ( ((ATX_UInt64)buffer[6])<<8  ) |
        ( ((ATX_UInt64)buffer[7])     );    
}

/*----------------------------------------------------------------------
|    ATX_BytesToInt16Be
+---------------------------------------------------------------------*/
ATX_UInt16 
ATX_BytesToInt16Be(const unsigned char* buffer)
{
    return 
        ( ((unsigned short)buffer[0])<<8  ) |
        ( ((unsigned short)buffer[1])     );
}

/*----------------------------------------------------------------------
|    ATX_BytesFromInt32Le
+---------------------------------------------------------------------*/
void 
ATX_BytesFromInt32Le(unsigned char* buffer, ATX_UInt32 value)
{
    buffer[3] = (unsigned char)(value>>24) & 0xFF;
    buffer[2] = (unsigned char)(value>>16) & 0xFF;
    buffer[1] = (unsigned char)(value>> 8) & 0xFF;
    buffer[0] = (unsigned char)(value    ) & 0xFF;
}

/*----------------------------------------------------------------------
|    ATX_BytesFromInt16Le
+---------------------------------------------------------------------*/
extern void 
ATX_BytesFromInt16Le(unsigned char* buffer, ATX_UInt16 value)
{
    buffer[1] = (unsigned char)((value>> 8) & 0xFF);
    buffer[0] = (unsigned char)((value    ) & 0xFF);
}

/*----------------------------------------------------------------------
|    ATX_BytesToInt32Le
+---------------------------------------------------------------------*/
extern ATX_UInt32 
ATX_BytesToInt32Le(const unsigned char* buffer)
{
    return 
        ( ((ATX_UInt32)buffer[3])<<24 ) |
        ( ((ATX_UInt32)buffer[2])<<16 ) |
        ( ((ATX_UInt32)buffer[1])<<8  ) |
        ( ((ATX_UInt32)buffer[0])     );
}

/*----------------------------------------------------------------------
|    ATX_BytesToInt16Le
+---------------------------------------------------------------------*/
extern ATX_UInt16 
ATX_BytesToInt16Le(const unsigned char* buffer)
{
    return 
        ( ((unsigned short)buffer[1])<<8  ) |
        ( ((unsigned short)buffer[0])     );
}

/*----------------------------------------------------------------------
|    ATX_ParseDouble
+---------------------------------------------------------------------*/
ATX_Result 
ATX_ParseDouble(const char* str, double* result, ATX_Boolean relaxed)
{
    ATX_Boolean  after_radix = ATX_FALSE;
    ATX_Boolean  negative    = ATX_FALSE;
    ATX_Boolean  empty       = ATX_TRUE;
    double       value       = 0.0;
    double       decimal     = 10.0;
    char         c;

    /* safe default value */
    *result = 0.0f;

    /* check params */
    if (str == NULL || *str == '\0') {
        return ATX_ERROR_INVALID_PARAMETERS;
    }

    /* ignore leading whitespace */
    if (relaxed) {
        while (ATX_IsSpace(*str)) {
            str++;
        }
    }
    if (str == NULL || *str == '\0') {
        return ATX_ERROR_INVALID_PARAMETERS;
    }

    /* check for sign */
    if (*str == '-') {
        /* negative number */
        negative = ATX_TRUE; 
        str++;
    } else if (*str == '+') {
        /* skip the + sign */
        str++;
    }

    while ((c = *str++)) {
        if (c == '.') {
            if (after_radix || (*str < '0' || *str > '9')) {
                return ATX_ERROR_INVALID_PARAMETERS;
            } else {
                after_radix = ATX_TRUE;
            }
        } else if (c >= '0' && c <= '9') {
            empty = ATX_FALSE;
            if (after_radix) {
                value += (double)(c-'0')/decimal;
                decimal *= 10.0f;
            } else {
                value = 10.0f*value + (double)(c-'0');
            }
        } else if (c == 'e' || c == 'E') {
            /* exponent */
            if (*str == '+' || *str == '-' || (*str >= '0' && *str <= '9')) {
                int exponent = 0;
                if (ATX_SUCCEEDED(ATX_ParseInteger(str, &exponent, relaxed))) {
                    value *= (double)pow(10.0f, (double)exponent);
                    break;
                } else {
                    return ATX_ERROR_INVALID_PARAMETERS;
                }
            } else {
                return ATX_ERROR_INVALID_PARAMETERS;
            }
        } else {
            if (relaxed) {
                break;
            } else {
                return ATX_ERROR_INVALID_PARAMETERS;
            }
        } 
    }

    /* check that the value was non empty */
    if (empty) {
        return ATX_ERROR_INVALID_PARAMETERS;
    }

    /* return the result */
    *result = negative ? -value : value;
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_ParseFloat
+---------------------------------------------------------------------*/
ATX_Result 
ATX_ParseFloat(const char* str, float* result, ATX_Boolean relaxed)
{
    double value;
    ATX_Result xr = ATX_ParseDouble(str, &value, relaxed);
    if (ATX_FAILED(xr)) {
        *result = 0.0f;
        return xr;
    }
    *result = (float)value;
    
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_ParseInteger64
+---------------------------------------------------------------------*/
ATX_Result 
ATX_ParseInteger64(const char* str, ATX_Int64* result, ATX_Boolean relaxed)
{
    ATX_Boolean negative = ATX_FALSE;
    ATX_Boolean empty    = ATX_TRUE;
    ATX_Int64   value    = 0;
    ATX_Int64   max      = ATX_INT64_MAX/10;
    char        c;

    /* safe default value */
    *result = 0;

    /* check params */
    if (str == NULL || *str == '\0') {
        return ATX_ERROR_INVALID_PARAMETERS;
    }

    /* ignore leading whitespace */
    if (relaxed) {
        while (ATX_IsSpace(*str)) {
            str++;
        }
    }
    if (*str == '\0') return ATX_ERROR_INVALID_PARAMETERS;

    /* check for sign */
    if (*str == '-') {
        /* negative number */
        negative = ATX_TRUE; 
        str++;
    } else if (*str == '+') {
        /* skip the + sign */
        str++;
    }

    /* adjust the max for overflows when the value is negative */
    if (negative && ((ATX_INT64_MAX%10) == 9)) ++max;

    while ((c = *str++)) {
        if (c >= '0' && c <= '9') {
            if (value < 0 || value > max) return ATX_ERROR_OVERFLOW;
            value = 10*value + (c-'0');
            if (value < 0 && (!negative || value != ATX_INT64_MIN)) return ATX_ERROR_OVERFLOW;
            empty = ATX_FALSE;
        } else {
            if (relaxed) {
                break;
            } else {
                return ATX_ERROR_INVALID_PARAMETERS;
            }
        } 
    }

    /* check that the value was non empty */
    if (empty) {
        return ATX_ERROR_INVALID_PARAMETERS;
    }

    /* return the result */
    if (negative) {
        *result = -value;
    } else {
        *result = value;
    }
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_ParseInteger64U
+---------------------------------------------------------------------*/
ATX_Result 
ATX_ParseInteger64U(const char* str, ATX_UInt64* result, ATX_Boolean relaxed)
{
    ATX_Boolean empty = ATX_TRUE;
    ATX_UInt64  value = 0;
    char        c;

    /* safe default value */
    *result = 0;

    /* check params */
    if (str == NULL || *str == '\0') {
        return ATX_ERROR_INVALID_PARAMETERS;
    }

    /* ignore leading whitespace */
    if (relaxed) {
        while (ATX_IsSpace(*str)) {
            str++;
        }
    }
    if (*str == '\0') return ATX_ERROR_INVALID_PARAMETERS;

    while ((c = *str++)) {
        if (c >= '0' && c <= '9') {
            ATX_UInt64 new_value;
            if (value > ATX_UINT64_MAX/10)  return ATX_ERROR_OVERFLOW;
            new_value = 10*value + (c-'0');
            if (new_value < value) return ATX_ERROR_OVERFLOW;
            value = new_value;
            empty = ATX_FALSE;
        } else {
            if (relaxed) {
                break;
            } else {
                return ATX_ERROR_INVALID_PARAMETERS;
            }
        } 
    }

    /* check that the value was non empty */
    if (empty) {
        return ATX_ERROR_INVALID_PARAMETERS;
    }

    /* return the result */
    *result = value;
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    ATX_ParseInteger32
+---------------------------------------------------------------------*/
ATX_Result 
ATX_ParseInteger32(const char* str, ATX_Int32* value, ATX_Boolean relaxed)
{
    ATX_Int64 value_64;
    ATX_Result result = ATX_ParseInteger64(str, &value_64, relaxed);
    *value = 0;
    if (ATX_SUCCEEDED(result)) {
        if (value_64 < ATX_INT_MIN || value_64 > ATX_INT_MAX) {
            return ATX_ERROR_OVERFLOW;
        }
        *value = (ATX_Int32)value_64;
    }
    return result;
}

/*----------------------------------------------------------------------
|    ATX_ParseInteger32U
+---------------------------------------------------------------------*/
ATX_Result 
ATX_ParseInteger32U(const char* str, ATX_UInt32* value, ATX_Boolean relaxed)
{
    ATX_UInt64 value_64;
    ATX_Result result = ATX_ParseInteger64U(str, &value_64, relaxed);
    *value = 0;
    if (ATX_SUCCEEDED(result)) {
        if (value_64 > ATX_UINT_MAX) return ATX_ERROR_OVERFLOW;
        *value = (ATX_UInt32)value_64;
    }
    return result;
}

/*----------------------------------------------------------------------
|    ATX_ParseInteger
+---------------------------------------------------------------------*/
ATX_Result 
ATX_ParseInteger(const char* str, int* value, ATX_Boolean relaxed)
{
    ATX_Int64 value_64;
    ATX_Result result = ATX_ParseInteger64(str, &value_64, relaxed);
    *value = 0;
    if (ATX_SUCCEEDED(result)) {
        if (value_64 < ATX_INT_MIN || value_64 > ATX_INT_MAX) {
            return ATX_ERROR_OVERFLOW;
        }
        *value = (int)value_64;
    }
    return result;
}

/*----------------------------------------------------------------------
|    ATX_ParseIntegerU
+---------------------------------------------------------------------*/
ATX_Result 
ATX_ParseIntegerU(const char* str, unsigned int* value, ATX_Boolean relaxed)
{
    ATX_UInt64 value_64;
    ATX_Result result = ATX_ParseInteger64U(str, &value_64, relaxed);
    *value = 0;
    if (ATX_SUCCEEDED(result)) {
        if (value_64 > ATX_UINT_MAX) return ATX_ERROR_OVERFLOW;
        *value = (unsigned int)value_64;
    }
    return result;
}

/*----------------------------------------------------------------------
|   ATX_DoubleToString
+---------------------------------------------------------------------*/
ATX_Result
ATX_DoubleToString(double value, char* buffer, ATX_Size buffer_size)
{
    char  s[256];
    char* c = s;

    /* check arguments */
    if (buffer_size < 4) return ATX_ERROR_OUT_OF_RANGE;

    /* deal with the sign */
    if (value < 0.0f) {
        value = -value;
        *c++ = '-';
    }

    if (value == 0.0f) {
        *c++ = '0';
    } else {
        double limit;
        do {
            ATX_Int32 integer_part;
            limit = 1.0f;
            while (value > limit*1E9f) {
                limit *= 1E9f;
            }
            /* convert the top of the integer part */
            integer_part = (ATX_Int32)(value/limit);
            ATX_IntegerToString(integer_part, c, (ATX_Size)(sizeof(s)-(c-&s[0])));
            while (*c != '\0') { ++c; }
            value -= limit*(double)integer_part;
        } while (limit > 1.0f);
    }

    /* emit the fractional part */
    if (value >= 1.0f) {
        *buffer = '\0';
        return ATX_ERROR_INTERNAL;
    }
    *c++ = '.';
    if (value <= 1E-6) {
        *c++ = '0';
        *c   = '\0';
    } else {
        ATX_Int32 factional_part = (ATX_Int32)(value*1E6);
        do {
            int digit = factional_part/100000;
            factional_part = 10*(factional_part-(digit*100000));
            *c++ = '0'+digit;
        } while (factional_part);
        *c = '\0';
    }

    /* copy the string */
    if (ATX_StringLength(s)+1 > buffer_size) {
        return ATX_ERROR_OUT_OF_RANGE;
    } else {
        ATX_CopyString(buffer, s);
        return ATX_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   ATX_FloatToString
+---------------------------------------------------------------------*/
ATX_Result
ATX_FloatToString(float value, char* buffer, ATX_Size buffer_size)
{
    return ATX_DoubleToString(value, buffer, buffer_size);
}

/*----------------------------------------------------------------------
|   ATX_IntegerToString
+---------------------------------------------------------------------*/
ATX_Result
ATX_IntegerToString(ATX_Int64 value, char* buffer, ATX_Size buffer_size)
{
    char s[32];
    char* c = &s[31];
    ATX_Boolean negative;
    *c-- = '\0';

    /* default value */
    if (buffer == NULL || buffer_size == 0) return ATX_ERROR_INVALID_PARAMETERS;
    buffer[0] = '\0';

    /* handle the sign */
    negative = ATX_FALSE;
    if (value < 0) {
        negative = ATX_TRUE;
        value = -value;
    }

    /* process the digits */
    do {
        int digit = (int)(value%10);
        *c-- = '0'+digit;
        value /= 10;
    } while(value);

    if (negative) {
        *c = '-';
    } else {
        ++c;
    }

    /* check that the string fits */
    if ((ATX_Size)(&s[31]-c)+1 > buffer_size) return ATX_ERROR_OUT_OF_RANGE;

    /* copy the string */
    ATX_CopyString(buffer, c);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_IntegerToStringU
+---------------------------------------------------------------------*/
ATX_Result
ATX_IntegerToStringU(ATX_UInt64 value, char* buffer, ATX_Size buffer_size)
{
    char s[32];
    char* c = &s[31];
    *c-- = '\0';

    /* default value */
    if (buffer == NULL || buffer_size == 0) return ATX_ERROR_INVALID_PARAMETERS;
    buffer[0] = '\0';

    /* process the digits */
    do {
        unsigned int digit = (unsigned int)(value%10);
        *c-- = '0'+digit;
        value /= 10;
    } while(value);
    ++c;

    /* check that the string fits */
    if ((ATX_Size)(&s[31]-c)+1 > buffer_size) return ATX_ERROR_OUT_OF_RANGE;

    /* copy the string */
    ATX_CopyString(buffer, c);

    return ATX_SUCCESS;
}

#if !defined(ATX_CONFIG_HAVE_STRCPY)
/*----------------------------------------------------------------------
|    ATX_CopyString
+---------------------------------------------------------------------*/
void
ATX_CopyString(char* dst, const char* src)
{
    while((*dst++ = *src++));
}
#endif

/*----------------------------------------------------------------------
|   ATX_FormatOutput
+---------------------------------------------------------------------*/
void
ATX_FormatOutput(void        (*function)(void* parameter, const char* message),
                 void*       function_parameter,
                 const char* format, 
                 va_list     args)
{
    char         local_buffer[ATX_FORMAT_LOCAL_BUFFER_SIZE];
    unsigned int buffer_size = ATX_FORMAT_LOCAL_BUFFER_SIZE;
    char*        buffer = local_buffer;

    for(;;) {
        int result;

        /* try to format the message (it might not fit) */
        result = ATX_FormatStringVN(buffer, buffer_size-1, format, args);
        buffer[buffer_size-1] = 0; /* force a NULL termination */
        if (result >= 0) break;

        /* the buffer was too small, try something bigger */
        buffer_size = (buffer_size+ATX_FORMAT_BUFFER_INCREMENT)*2;
        if (buffer_size > ATX_FORMAT_BUFFER_MAX_SIZE) break;
        if (buffer != local_buffer) ATX_FreeMemory((void*)buffer);
        buffer = ATX_AllocateMemory(buffer_size);
        if (buffer == NULL) return;
    }

    (*function)(function_parameter, buffer);
    if (buffer != local_buffer) ATX_FreeMemory((void*)buffer);
}

/*----------------------------------------------------------------------
|   ATX_ScrubMemory
+---------------------------------------------------------------------*/
void 
ATX_ScrubMemory(void* buffer, ATX_Size size)
{
    if (buffer == NULL) return;

    {
        volatile char* mem = (volatile char*)buffer; 
    
        while (size--) {
            *mem++ = 0; 
        }
    }
}

/*----------------------------------------------------------------------
|   ATX_NibbleToHex
+---------------------------------------------------------------------*/
char
ATX_NibbleToHex(unsigned int nibble, ATX_Boolean uppercase)
{
    ATX_ASSERT(nibble < 16);
    if (uppercase) {
        return (nibble < 10) ? ('0' + nibble) : ('A' + (nibble-10));
    } else {
        return (nibble < 10) ? ('0' + nibble) : ('a' + (nibble-10));
    }
    return (nibble < 10) ? ('0' + nibble) : ('A' + (nibble-10));
}

/*----------------------------------------------------------------------
|   ATX_ByteToHex
+---------------------------------------------------------------------*/
void
ATX_ByteToHex(ATX_Byte b, char* buffer, ATX_Boolean uppercase)
{
    buffer[0] = ATX_NibbleToHex((b>>4) & 0x0F, uppercase);
    buffer[1] = ATX_NibbleToHex(b      & 0x0F, uppercase);
}

/*----------------------------------------------------------------------
|   ATX_HexString
+---------------------------------------------------------------------*/
ATX_String
ATX_HexString(const unsigned char* data,
              ATX_Size             data_size,
              ATX_Boolean          uppercase)
{
    ATX_String           result;
    const unsigned char* src = data;
    char*                dst;
    ATX_INIT_STRING(result);
    
    /* quick check */
    if (data == NULL || data_size == 0) return result;
    
    /* set the result size */
    ATX_String_Reserve(&result, 2*data_size);
    
    /* build the string */
    dst = ATX_String_UseChars(&result);
    dst[2*data_size] = '\0'; /* NULL terminate */
    while (data_size--) {
        ATX_ByteToHex(*src++, dst, uppercase);
        dst += 2;
    }
    
    
    return result;
}

/*----------------------------------------------------------------------
|   ATX_HexToNibble
+---------------------------------------------------------------------*/
int 
ATX_HexToNibble(char hex)
{
    if (hex >= 'a' && hex <= 'f') {
        return ((hex - 'a') + 10);
    } else if (hex >= 'A' && hex <= 'F') {
        return ((hex - 'A') + 10);
    } else if (hex >= '0' && hex <= '9') {
        return (hex - '0');
    } else {
        return -1;
    }
}

/*----------------------------------------------------------------------
|   ATX_HexToByte
+---------------------------------------------------------------------*/
ATX_Result
ATX_HexToByte(const char* buffer, ATX_Byte* b)
{
    int nibble_0 = ATX_HexToNibble(buffer[0]);
    int nibble_1 = ATX_HexToNibble(buffer[1]);
    
    if (nibble_0 < 0 || nibble_1 < 0) {
        return ATX_ERROR_INVALID_SYNTAX;
    }
    
    *b = (nibble_0 << 4) | nibble_1;
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_HexToBytes
+---------------------------------------------------------------------*/
ATX_Result
ATX_HexToBytes(const char*     hex,
               ATX_DataBuffer* bytes)
{
    ATX_Size     byte_count;
    unsigned int i;
    ATX_Result   result;
    
    /* check the size */
    ATX_Size len = ATX_StringLength(hex);
    if ((len%2) != 0) return ATX_ERROR_INVALID_PARAMETERS;
    byte_count = len / 2;
    result = ATX_DataBuffer_SetDataSize(bytes, byte_count);
    if (ATX_FAILED(result)) return result;
    
    /* decode */
    for (i=0; i<byte_count; i++) {
        result = ATX_HexToByte(hex+(i*2), ATX_DataBuffer_UseData(bytes)+i);
        if (ATX_FAILED(result)) return result;
    }
    return ATX_SUCCESS;
}
