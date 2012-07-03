/*****************************************************************
|
|   Fluo - Runtime Utilities
|
|   (c) 2002-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _FLO_UTILS_H_
#define _FLO_UTILS_H_

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "FloConfig.h"
#include "FloTypes.h"

#if defined(FLO_CONFIG_HAVE_STDLIB_H)
#include <stdlib.h>
#endif /* FLO_CONFIG_HAVE_STDLIB_H */

#if defined(DMALLOC)
#include <dmalloc.h>
#endif

/*----------------------------------------------------------------------
|    macros
+---------------------------------------------------------------------*/
#define FLO_ARRAY_SIZE(x) (sizeof((x))/sizeof((x)[0]))

/*----------------------------------------------------------------------
|    C Runtime
+---------------------------------------------------------------------*/
#if defined(FLO_CONFIG_HAVE_MALLOC)
#define FLO_AllocateMemory malloc
#else
extern void* FLO_AllocateMemory(unsigned int);
#endif

#if defined(FLO_CONFIG_HAVE_CALLOC)
#define FLO_AllocateZeroMemory(x) calloc(1,(x))
#else
extern void* FLO_AllocateZeroMemory(unsigned int);
#endif

#if defined(FLO_CONFIG_HAVE_FREE)
#define FLO_FreeMemory free
#else
extern void FLO_FreeMemory(void* pointer);
#endif

#if defined(FLO_CONFIG_HAVE_MEMCPY)
#define FLO_CopyMemory memcpy
#else
extern void FLO_CopyMemory(void* dest, const void* src, FLO_Size size);
#endif

#if defined(FLO_CONFIG_HAVE_MEMMOVE)
#define FLO_MoveMemory memmove
#else
extern void FLO_MoveMemory(void* dest, const void* src, FLO_Size size);
#endif

#if defined(FLO_CONFIG_HAVE_MEMSET)
#define FLO_SetMemory memset
#else
extern void FLO_SetMemory(void* dest, int c, FLO_Size size);
#endif


#endif /* _FLO_UTILS_H_ */
