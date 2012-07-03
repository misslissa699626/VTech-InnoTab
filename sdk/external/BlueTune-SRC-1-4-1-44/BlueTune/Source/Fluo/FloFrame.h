/*****************************************************************
|
|   Fluo - Frames
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Fluo - Frames
 */

#ifndef _FLO_FRAME_H_
#define _FLO_FRAME_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "FloTypes.h"
#include "FloSyntax.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct {
    unsigned char id;
    unsigned char layer;
    unsigned char protection_bit;
    unsigned char bitrate_index;
    unsigned char sampling_frequency;
    unsigned char padding_bit;
    unsigned char private_bit;
    unsigned char mode;
    unsigned char mode_extension;
    unsigned char copyright;
    unsigned char original;
    unsigned char emphasis;
} FLO_FrameHeader;

typedef struct {
    FLO_MpegLevel level;
    FLO_MpegLayer layer;
    FLO_MpegMode  mode;
    FLO_UInt32    sample_rate;
    FLO_UInt32    bitrate;
    FLO_Size      size;
    FLO_Cardinal  channel_count;
    FLO_Cardinal  sample_count;
} FLO_FrameInfo;

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
extern void       
FLO_FrameHeader_GetInfo(FLO_FrameHeader* header, FLO_FrameInfo* info);
extern void       
FLO_FrameHeader_Unpack(FLO_UInt32 packed, FLO_FrameHeader* header);

extern void
FLO_FrameHeader_FromBytes(const FLO_Byte* bytes, FLO_FrameHeader* header);

extern FLO_Result 
FLO_FrameHeader_Check(FLO_FrameHeader* header);

#endif /* _FLO_FRAME_H_ */
