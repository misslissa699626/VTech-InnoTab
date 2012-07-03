/*****************************************************************
|
|   BlueTune - General Types
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _FLO_TYPES_H_
#define _FLO_TYPES_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "AtxTypes.h"

/*----------------------------------------------------------------------
|   import some Atomix types
+---------------------------------------------------------------------*/
typedef ATX_UInt32       FLO_UInt32;
typedef ATX_Int32        FLO_Int32;
typedef ATX_UInt16       FLO_UInt16;
typedef ATX_Int16        FLO_Int16;
typedef ATX_UInt8        FLO_UInt8;
typedef ATX_Int8         FLO_Int8;
typedef ATX_UInt8        FLO_Byte;

typedef ATX_Result       FLO_Result;
typedef ATX_Boolean      FLO_Boolean;
typedef ATX_Flags        FLO_Flags;
typedef ATX_Mask         FLO_Mask;
typedef ATX_Offset       FLO_Offset;
typedef ATX_Address      FLO_Address;
typedef ATX_Range        FLO_Range;
typedef ATX_Cardinal     FLO_Cardinal;
typedef ATX_Ordinal      FLO_Ordinal;
typedef ATX_CString      FLO_CString;
typedef ATX_Any          FLO_Any;
typedef ATX_AnyConst     FLO_AnyConst;
typedef ATX_ByteBuffer   FLO_ByteBuffer;
typedef ATX_Size         FLO_Size;
typedef ATX_Int64        FLO_Int64;

#define FLO_TRUE         ATX_TRUE
#define FLO_FALSE        ATX_FALSE

#endif /* _FLO_TYPES_H_ */
