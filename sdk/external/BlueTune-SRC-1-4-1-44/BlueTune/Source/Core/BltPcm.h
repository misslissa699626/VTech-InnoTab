/*****************************************************************
|
|   BlueTune - PCM Utilities
|
|   (c) 2002-2009 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * PCM API
 */

#ifndef _BLT_PCM_H_
#define _BLT_PCM_H_

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
typedef struct {
    BLT_MediaType base;
    BLT_UInt32    sample_rate;
    BLT_UInt16    channel_count;
    BLT_UInt8     bits_per_sample;
    BLT_UInt8     sample_format;
    BLT_UInt32    channel_mask;
} BLT_PcmMediaType;

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#define BLT_PCM_MEDIA_TYPE_EXTENSION_CLEAR(_e)  \
do {                                            \
    _e.sample_rate = 0;                         \
    _e.channel_count = 0;                       \
    _e.bits_per_sample = 0;                     \
    _e.sample_format = 0;                       \
} while(0);

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/

/* PCM sample format IDs   */
/* _NE means Native Endian */
/* _LE means Little Endian */
/* _BE means Big Endian    */

#define BLT_PCM_SAMPLE_FORMAT_NONE             0

#define BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_BE    1
#define BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_LE    2
#define BLT_PCM_SAMPLE_FORMAT_UNSIGNED_INT_BE  3
#define BLT_PCM_SAMPLE_FORMAT_UNSIGNED_INT_LE  4
#define BLT_PCM_SAMPLE_FORMAT_FLOAT_BE         5
#define BLT_PCM_SAMPLE_FORMAT_FLOAT_LE         6

#if BLT_CONFIG_CPU_BYTE_ORDER == BLT_CPU_BIG_ENDIAN
#define BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE    BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_BE
#define BLT_PCM_SAMPLE_FORMAT_UNSIGNED_INT_NE  BLT_PCM_SAMPLE_FORMAT_UNSIGNED_INT_BE
#define BLT_PCM_SAMPLE_FORMAT_FLOAT_NE         BLT_PCM_SAMPLE_FORMAT_FLOAT_BE
#else
#define BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE    BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_LE
#define BLT_PCM_SAMPLE_FORMAT_UNSIGNED_INT_NE  BLT_PCM_SAMPLE_FORMAT_UNSIGNED_INT_LE
#define BLT_PCM_SAMPLE_FORMAT_FLOAT_NE         BLT_PCM_SAMPLE_FORMAT_FLOAT_LE
#endif

/* NOTE: the PCM channel order (interleaving in the buffer) is as follows:
1.  Front Left - FL
2.  Front Right - FR
3.  Front Center - FC
4.  Low Frequency - LF
5.  Back Left - BL
6.  Back Right - BR
7.  Front Left of Center - FLC
8.  Front Right of Center - FRC
9.  Back Center - BC
10. Side Left - SL
11. Side Right - SR
12. Top Center - TC
13. Top Front Left - TFL
14. Top Front Center - TFC
15. Top Front Right - TFR
16. Top Back Left - TBL
17. Top Back Center - TBC
18. Top Back Right - TBR
*/

/* channel mask bits */
#define BLT_PCM_SPEAKER_FRONT_LEFT 	          (1    )
#define BLT_PCM_SPEAKER_FRONT_RIGHT 	      (1<< 1)
#define BLT_PCM_SPEAKER_FRONT_CENTER 	      (1<< 2)
#define BLT_PCM_SPEAKER_LOW_FREQUENCY 	      (1<< 3)
#define BLT_PCM_SPEAKER_BACK_LEFT 	          (1<< 4)
#define BLT_PCM_SPEAKER_BACK_RIGHT 	          (1<< 5)
#define BLT_PCM_SPEAKER_FRONT_LEFT_OF_CENTER  (1<< 6)
#define BLT_PCM_SPEAKER_FRONT_RIGHT_OF_CENTER (1<< 7)
#define BLT_PCM_SPEAKER_BACK_CENTER 	      (1<< 8)
#define BLT_PCM_SPEAKER_SIDE_LEFT 	          (1<< 9)
#define BLT_PCM_SPEAKER_SIDE_RIGHT 	          (1<<10)
#define BLT_PCM_SPEAKER_TOP_CENTER 	          (1<<11)
#define BLT_PCM_SPEAKER_TOP_FRONT_LEFT 	      (1<<12)
#define BLT_PCM_SPEAKER_TOP_FRONT_CENTER 	  (1<<13)
#define BLT_PCM_SPEAKER_TOP_FRONT_RIGHT 	  (1<<14)
#define BLT_PCM_SPEAKER_TOP_BACK_LEFT 	      (1<<15)
#define BLT_PCM_SPEAKER_TOP_BACK_CENTER 	  (1<<16)
#define BLT_PCM_SPEAKER_TOP_BACK_RIGHT 	      (1<<17)

/* predefined channel masks */
#define BLT_CHANNEL_MASK_MONO            (BLT_PCM_SPEAKER_FRONT_CENTER)
#define BLT_CHANNEL_MASK_STEREO          (BLT_PCM_SPEAKER_FRONT_LEFT | BLT_PCM_SPEAKER_FRONT_RIGHT)
#define BLT_CHANNEL_MASK_QUAD            (BLT_PCM_SPEAKER_FRONT_LEFT | BLT_PCM_SPEAKER_FRONT_RIGHT | \
                                          BLT_PCM_SPEAKER_BACK_LEFT  | BLT_PCM_SPEAKER_BACK_RIGHT)
#define BLT_CHANNEL_MASK_SURROUND        (BLT_PCM_SPEAKER_FRONT_LEFT   | BLT_PCM_SPEAKER_FRONT_RIGHT | \
                                          BLT_PCM_SPEAKER_FRONT_CENTER | BLT_PCM_SPEAKER_BACK_CENTER)
#define BLT_CHANNEL_MASK_5POINT1         (BLT_PCM_SPEAKER_FRONT_LEFT   | BLT_PCM_SPEAKER_FRONT_RIGHT   | \
                                          BLT_PCM_SPEAKER_FRONT_CENTER | BLT_PCM_SPEAKER_LOW_FREQUENCY | \
                                          BLT_PCM_SPEAKER_BACK_LEFT    | BLT_PCM_SPEAKER_BACK_RIGHT)
#define BLT_CHANNEL_MASK_7POINT1         (BLT_PCM_SPEAKER_FRONT_LEFT           | BLT_PCM_SPEAKER_FRONT_RIGHT   | \
                                          BLT_PCM_SPEAKER_FRONT_CENTER         | BLT_PCM_SPEAKER_LOW_FREQUENCY | \
                                          BLT_PCM_SPEAKER_BACK_LEFT            | BLT_PCM_SPEAKER_BACK_RIGHT    | \
                                          BLT_PCM_SPEAKER_FRONT_LEFT_OF_CENTER | BLT_PCM_SPEAKER_FRONT_RIGHT_OF_CENTER)
#define BLT_CHANNEL_MASK_5POINT1_SURROUND (BLT_PCM_SPEAKER_FRONT_LEFT   | BLT_PCM_SPEAKER_FRONT_RIGHT   | \
                                           BLT_PCM_SPEAKER_FRONT_CENTER | BLT_PCM_SPEAKER_LOW_FREQUENCY | \
                                           BLT_PCM_SPEAKER_SIDE_LEFT    | BLT_PCM_SPEAKER_SIDE_RIGHT)
#define BLT_CHANNEL_MASK_7POINT1_SURROUND (BLT_PCM_SPEAKER_FRONT_LEFT   | BLT_PCM_SPEAKER_FRONT_RIGHT   | \
                                           BLT_PCM_SPEAKER_FRONT_CENTER | BLT_PCM_SPEAKER_LOW_FREQUENCY | \
                                           BLT_PCM_SPEAKER_BACK_LEFT    | BLT_PCM_SPEAKER_BACK_RIGHT    | \
                                           BLT_PCM_SPEAKER_SIDE_LEFT    | BLT_PCM_SPEAKER_SIDE_RIGHT)


/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/
extern const BLT_MediaType BLT_GenericPcmMediaType;

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void
BLT_PcmMediaType_Init(BLT_PcmMediaType* media_type);

extern BLT_Boolean
BLT_Pcm_CanConvert(const BLT_MediaType* from, const BLT_MediaType* to);

extern BLT_Result
BLT_Pcm_ConvertMediaPacket(BLT_Core*         core,
                           BLT_MediaPacket*  in_packet, 
                           BLT_PcmMediaType* out_type, 
                           BLT_MediaPacket** out_packet);

extern BLT_Result
BLT_Pcm_ParseMimeType(const char* mime_type, BLT_PcmMediaType** media_type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BLT_PCM_H_ */
