/*****************************************************************
|
|   BlueTune - Types
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Common Types
 */

#ifndef _BLT_TYPES_H_
#define _BLT_TYPES_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltDefs.h"

/*----------------------------------------------------------------------
|   import some Atomix types
+---------------------------------------------------------------------*/
typedef ATX_UInt64       BLT_UInt64;
typedef ATX_UInt32       BLT_UInt32;
typedef ATX_Int32        BLT_Int32;
typedef ATX_UInt16       BLT_UInt16;
typedef ATX_Int16        BLT_Int16;
typedef ATX_UInt8        BLT_UInt8;
typedef ATX_Int8         BLT_Int8;

typedef ATX_Result       BLT_Result;
typedef ATX_Flags        BLT_Flags;
typedef ATX_Mask         BLT_Mask;
typedef ATX_Offset       BLT_Offset;
typedef ATX_Position     BLT_Position;
typedef ATX_Address      BLT_Address;
typedef ATX_Range        BLT_Range;
typedef ATX_Cardinal     BLT_Cardinal;
typedef ATX_Ordinal      BLT_Ordinal;
typedef ATX_CString      BLT_CString;
typedef ATX_String       BLT_String;
typedef ATX_Any          BLT_Any;
typedef ATX_AnyConst     BLT_AnyConst;
typedef ATX_ByteBuffer   BLT_ByteBuffer;
typedef ATX_Size         BLT_Size;
typedef ATX_LargeSize    BLT_LargeSize;
typedef ATX_Boolean      BLT_Boolean;

/*----------------------------------------------------------------------
|   import some Neptune types
+---------------------------------------------------------------------*/
#ifdef __cplusplus

#include "Neptune.h"
typedef NPT_String BLT_StringObject;

#endif /* __cplusplus */

#endif /* _BLT_TYPES_H_ */
