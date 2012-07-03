/*****************************************************************
|
|    Melo - Debug Support
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
/** @file
 * Header file for debug & logging
 */

#ifndef _MLO_DEBUG_H_
#define _MLO_DEBUG_H_

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/

#include "MloConfig.h"

#if defined(MLO_CONFIG_HAVE_ASSERT_H)
#include <assert.h>
#endif

/*----------------------------------------------------------------------
|    Macros
+---------------------------------------------------------------------*/

/* Check a constant expression to make the compiler fail if false.
   Requires a ";" at the end */
#define  MLO_CHECK_CST(name, cond)  typedef int MLO_CheckCst_##name [(cond) ? 1 : -1]

/*----------------------------------------------------------------------
|    prototypes
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void MLO_Debug(const char* format, ...);
#define MLO_ASSERT(_x) assert(_x)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MLO_DEBUG_H_ */
