/*****************************************************************
|
|   BlueTune - Video Pixels
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Pixels API
 */

#ifndef _BLT_PIXELS_H_
#define _BLT_PIXELS_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltDefs.h"
#include "BltTypes.h"
#include "BltErrors.h"
#include "BltMedia.h"
#include "BltMediaPacket.h"
#include "BltCore.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef enum {
    BLT_PIXEL_FORMAT_YV12  /* Planar YUV 4:2:0 */
} BLT_PixelFormat;

typedef struct {
    BLT_MediaType   base;
    BLT_UInt16      width;
    BLT_UInt16      height;
    BLT_PixelFormat format;
    BLT_UInt32      flags;
    struct {
        BLT_UInt16 bytes_per_line;
        BLT_UInt32 offset;
    }               planes[4];
} BLT_RawVideoMediaType;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void
BLT_RawVideoMediaType_Init(BLT_RawVideoMediaType* media_type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BLT_PIXELS_H_ */
