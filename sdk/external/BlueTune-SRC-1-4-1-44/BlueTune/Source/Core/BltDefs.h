/*****************************************************************
|
|   BlueTune - Common Definitions
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Common preprocessor definitions
 */

#ifndef _BLT_DEFS_H_
#define _BLT_DEFS_H_

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#ifndef NULL
#define NULL ((void*)0)
#endif

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#define BLT_SAFE_STRING(s) ((s) == NULL ? "" : s)

/*----------------------------------------------------------------------
|   import some Atomix definitions
+---------------------------------------------------------------------*/
#define BLT_METHOD        ATX_METHOD
#define BLT_VOID_METHOD   ATX_VOID_METHOD
#define BLT_DIRECT_METHOD ATX_DIRECT_METHOD
#define BLT_TRUE          ATX_TRUE
#define BLT_FALSE         ATX_FALSE

#endif /* _BLT_DEFS_H_ */
