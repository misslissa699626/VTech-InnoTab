/*****************************************************************
|
|   BlueTune - MediaPort Objects
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BlueTune MediaNode Objects
 */

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltTypes.h"
#include "BltDefs.h"
#include "BltErrors.h"
#include "BltCore.h"
#include "BltMediaPort.h"

/*----------------------------------------------------------------------
|    BLT_MediaPort_DefaultQueryMediaType
+---------------------------------------------------------------------*/
BLT_Result 
BLT_MediaPort_DefaultQueryMediaType(BLT_MediaPort*        self,
                                    BLT_Ordinal           index,
                                    const BLT_MediaType** media_type)
{
    BLT_COMPILER_UNUSED(self);
    BLT_COMPILER_UNUSED(index);
    BLT_COMPILER_UNUSED(media_type);
    return BLT_ERROR_NOT_IMPLEMENTED;
}
