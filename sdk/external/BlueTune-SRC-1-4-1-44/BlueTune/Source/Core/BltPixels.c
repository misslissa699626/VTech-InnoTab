/*****************************************************************
|
|   BlueTune - Video Pixels
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Video Pixels Implementation file
 */

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "BltPixels.h"

/*----------------------------------------------------------------------
|   BLT_RawVideoMediaType_Init
+---------------------------------------------------------------------*/
void
BLT_RawVideoMediaType_Init(BLT_RawVideoMediaType* media_type)
{
    ATX_SetMemory(media_type, 0, sizeof(*media_type));
    media_type->base.id = BLT_MEDIA_TYPE_ID_VIDEO_RAW;
    media_type->base.extension_size = sizeof(BLT_RawVideoMediaType)-sizeof(BLT_MediaType);
}

