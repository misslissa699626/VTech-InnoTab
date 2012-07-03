/*****************************************************************
|
|    Melo - Configuration
|
|    Copyright 2002-2006 Axiomatic Systems LLC
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
/** @file
 * Melo Configuration
 */

#ifndef _MLO_CONFIG_H_
#define _MLO_CONFIG_H_

/*----------------------------------------------------------------------
|    defaults
+---------------------------------------------------------------------*/
#define MLO_CONFIG_HAVE_STD_C
#define MLO_CONFIG_HAVE_STDLIB_H
#define MLO_CONFIG_HAVE_STRING_H
#define MLO_CONFIG_HAVE_CTYPE_H
#define MLO_CONFIG_HAVE_MATH_H
#define MLO_CONFIG_HAVE_ASSERT_H
#define MLO_CONFIG_HAVE_LIMITS_H

/*----------------------------------------------------------------------
|       base types
+---------------------------------------------------------------------*/
typedef int              MLO_Int32;
typedef unsigned int     MLO_UInt32;
typedef short            MLO_Int16;
typedef unsigned short   MLO_UInt16;
typedef signed char      MLO_Int8;
typedef unsigned char    MLO_UInt8;

/*----------------------------------------------------------------------
|    platform specifics
+---------------------------------------------------------------------*/
/* Windows 32 */
#if defined(_MSC_VER)
#if !defined(STRICT)
#define STRICT
#define inline __inline
#endif

#define vsnprintf _vsnprintf

#endif

/* Microsoft C/C++ Compiler */
#if defined(_MSC_VER)
#define MLO_CONFIG_HAVE_INT64
#define MLO_CONFIG_INT64_TYPE __int64
#endif

/* QNX */
#if defined(__QNX__)
#endif

/*----------------------------------------------------------------------
|    compiler specifics
+---------------------------------------------------------------------*/
/* GCC */
#if defined(__GNUC__)
#define MLO_COMPILER_UNUSED(p) (void)p
#define MLO_CONFIG_HAVE_INT64
#define MLO_CONFIG_INT64_TYPE long long
#else
#define MLO_COMPILER_UNUSED(p) 
#endif

/*----------------------------------------------------------------------
|    standard C runtime
+---------------------------------------------------------------------*/
#if defined (MLO_CONFIG_HAVE_STD_C)
#define MLO_CONFIG_HAVE_MALLOC
#define MLO_CONFIG_HAVE_CALLOC
#define MLO_CONFIG_HAVE_REALLOC
#define MLO_CONFIG_HAVE_FREE
#define MLO_CONFIG_HAVE_MEMCPY
#define MLO_CONFIG_HAVE_MEMMOVE
#define MLO_CONFIG_HAVE_MEMSET
#endif /* MLO_CONFIG_HAS_STD_C */

#if defined (MLO_CONFIG_HAVE_STRING_H)
#define MLO_CONFIG_HAVE_STRCMP
#define MLO_CONFIG_HAVE_STRNCMP
#define MLO_CONFIG_HAVE_STRDUP
#define MLO_CONFIG_HAVE_STRLEN
#define MLO_CONFIG_HAVE_STRCPY
#define MLO_CONFIG_HAVE_STRNCPY
#endif /* MLO_CONFIG_HAVE_STRING_H */

#if defined (MLO_CONFIG_HAVE_MATH_H)
#define MLO_CONFIG_HAVE_POW
#define MLO_CONFIG_HAVE_SQRT
#define MLO_CONFIG_HAVE_COS
#define MLO_CONFIG_HAVE_SIN
#endif /* MLO_CONFIG_HAVE_MATH_H */

#endif /* _MLO_CONFIG_H_ */
