/*****************************************************************
|
|    Melo - Runtime Utilities
|
|    Copyright 2004-2006 Axiomatic Systems LLC
|
|    This file is part of Melo (Melo AAC Decoder).
|
|    Unless you have obtained Melo under a difference license,
|    this version of Melo is Melo|GPL.
|    Melo|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Melo|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Melo|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
 ****************************************************************/

#ifndef _MLO_UTILS_H_
#define _MLO_UTILS_H_

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "MloConfig.h"
#include "MloTypes.h"

#if defined(MLO_CONFIG_HAVE_STDLIB_H)
#include <stdlib.h>
#endif /* MLO_CONFIG_HAVE_STDLIB_H */

#if defined(MLO_CONFIG_HAVE_STRING_H)
#include <string.h>
#endif /* MLO_CONFIG_HAVE_STRING_H */

#if defined (MLO_CONFIG_HAVE_LIMITS_H)
#include <limits.h>
#endif   /* MLO_CONFIG_HAVE_LIMITS_H */

#if defined(DMALLOC)
#include <dmalloc.h>
#endif

/*----------------------------------------------------------------------
|    macros
+---------------------------------------------------------------------*/
#define  MLO_ARRAY_SIZE(x) (sizeof((x))/sizeof((x)[0]))

#if ! defined (MLO_CONFIG_HAVE_LIMITS_H) && ! defined (CHAR_BIT)
#define  CHAR_BIT          (8)
#endif   /* MLO_CONFIG_HAVE_LIMITS_H */
#define  MLO_BIT_DEPTH(x)  (sizeof (x) * CHAR_BIT)
#define  MLO_MAX_VAL_U(x)  ((1L << MLO_BIT_DEPTH (x)) - 1)
#define  MLO_MAX_VAL_S(x)  ((1L << (MLO_BIT_DEPTH (x) - 1)) - 1)
#define  MLO_MIN_VAL_S(x)  (-1L << (MLO_BIT_DEPTH (x) - 1))

#define  MLO_ABS(x)        (((x) < 0) ? (-(x)) : (x))
#define  MLO_SIGN(x)       (((x) < 0) ? -1 : 1)
#define  MLO_MAX(x,a)      (((x) < (a)) ? (a) : (x))
#define  MLO_MIN(x,a)      (((a) < (x)) ? (a) : (x))
#define  MLO_BOUND(x,a,b)  (MLO_MAX (MLO_MIN ((x), (b)), (a)))

/*----------------------------------------------------------------------
|    byte I/O
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void MLO_BytesFromInt32Be(unsigned char* buffer, unsigned long value);
extern void MLO_BytesFromInt16Be(unsigned char* buffer, unsigned short value);
extern unsigned long MLO_BytesToInt32Be(const unsigned char* buffer);
extern unsigned short MLO_BytesToInt16Be(const unsigned char* buffer);

extern void MLO_BytesFromInt32Le(unsigned char* buffer, unsigned long value);
extern void MLO_BytesFromInt16Le(unsigned char* buffer, unsigned short value);
extern unsigned long MLO_BytesToInt32Le(const unsigned char* buffer);
extern unsigned short MLO_BytesToInt16Le(const unsigned char* buffer);

/*----------------------------------------------------------------------
|    C Runtime
+---------------------------------------------------------------------*/
#if defined(MLO_CONFIG_HAVE_MALLOC)
#define MLO_AllocateMemory malloc
#else
extern void* MLO_AllocateMemory(unsigned int);
#endif

#if defined(MLO_CONFIG_HAVE_CALLOC)
#define MLO_AllocateZeroMemory(x) calloc(1,(x))
#else
extern void* MLO_AllocateZeroMemory(unsigned int);
#endif

#if defined(MLO_CONFIG_HAVE_FREE)
#define MLO_FreeMemory free
#else
extern void MLO_FreeMemory(void* pointer);
#endif

#if defined(MLO_CONFIG_HAVE_MEMCPY)
#define MLO_CopyMemory memcpy
#else
extern void MLO_CopyMemory(void* dest, const void* src, MLO_Size size);
#endif

#if defined(MLO_CONFIG_HAVE_MEMMOVE)
#define MLO_MoveMemory memmove
#else
extern void MLO_MoveMemory(void* dest, const void* src, MLO_Size size);
#endif

#if defined(MLO_CONFIG_HAVE_MEMSET)
#define MLO_SetMemory memset
#else
extern void MLO_SetMemory(void* dest, int c, MLO_Size size);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MLO_UTILS_H_ */
