/*****************************************************************
|
|   Fluo - MPEG Syntax
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Fluo - MPEG Syntax
 */

#ifndef _FLO_SYNTAX_H_
#define _FLO_SYNTAX_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "FloTypes.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef enum {
    FLO_MPEG_LEVEL_MPEG_2   = 0,
    FLO_MPEG_LEVEL_MPEG_1   = 1,
    FLO_MPEG_LEVEL_MPEG_2_5 = 2,
    FLO_MPEG_LEVEL_ILLEGAL  = 3
} FLO_MpegLevel;

typedef enum {
    FLO_MPEG_RESERVED  = 0,
    FLO_MPEG_LAYER_I   = 1,
    FLO_MPEG_LAYER_II  = 2,
    FLO_MPEG_LAYER_III = 3
} FLO_MpegLayer;

typedef enum {
    FLO_MPEG_MODE_STEREO         = 0,
    FLO_MPEG_MODE_JOINT_STEREO   = 1,
    FLO_MPEG_MODE_DUAL_CHANNEL   = 2,
    FLO_MPEG_MODE_SINGLE_CHANNEL = 3
} FLO_MpegMode;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define FLO_SYNTAX_MPEG_SYNC_WORD_BIT_LENGTH               11
#define FLO_SYNTAX_MPEG_SYNC_WORD                          0x000007FFL

#define FLO_SYNTAX_MPEG_LAYER_I_BYTES_PER_SLOT             4

#define FLO_SYNTAX_MPEG_ID_MPEG_2                          0
#define FLO_SYNTAX_MPEG_ID_MPEG_1                          1
#define FLO_SYNTAX_MPEG_ID_MPEG_2_5                        2
#define FLO_SYNTAX_MPEG_ID_ILLEGAL                         3

#define FLO_SYNTAX_MPEG_MODE_STEREO                        0
#define FLO_SYNTAX_MPEG_MODE_JOINT_STEREO                  1
#define FLO_SYNTAX_MPEG_MODE_DUAL_CHANNEL                  2
#define FLO_SYNTAX_MPEG_MODE_SINGLE_CHANNEL                3

#define FLO_SYNTAX_MPEG_LAYER_RESERVED                     0
#define FLO_SYNTAX_MPEG_LAYER_III                          1
#define FLO_SYNTAX_MPEG_LAYER_II                           2
#define FLO_SYNTAX_MPEG_LAYER_I                            3

#define FLO_SYNTAX_MPEG_LAYER_I_ALLOCATION_INVALID         15
#define FLO_SYNTAX_MPEG_LAYER_I_II_SCALEFACTOR_INVALID     63

#define FLO_SYNTAX_MPEG_FRAME_HAS_CRC                      0
#define FLO_SYNTAX_MPEG_FRAME_HAS_NO_CRC                   1
#define FLO_SYNTAX_MPEG_CRC_SIZE                           16

#define FLO_SYNTAX_MPEG_SAMPLING_FREQUENCY_44100_22050     0
#define FLO_SYNTAX_MPEG_SAMPLING_FREQUENCY_48000_24000     1
#define FLO_SYNTAX_MPEG_SAMPLING_FREQUENCY_32000_16000     2
#define FLO_SYNTAX_MPEG_SAMPLING_FREQUENCY_RESERVED        3

#define FLO_SYNTAX_MPEG_EMPHASIS_NONE                      0
#define FLO_SYNTAX_MPEG_EMPHASIS_50_15                     1
#define FLO_SYNTAX_MPEG_EMPHASIS_RESERVED                  2
#define FLO_SYNTAX_MPEG_EMPHASIS_CCITT_J17                 3

#define FLO_SYNTAX_MPEG_BITRATE_FREE_FORMAT                0

#define FLO_SYNTAX_MPEG_BITRATE_I_32                       1
#define FLO_SYNTAX_MPEG_BITRATE_I_64                       2
#define FLO_SYNTAX_MPEG_BITRATE_I_96                       3
#define FLO_SYNTAX_MPEG_BITRATE_I_128                      4
#define FLO_SYNTAX_MPEG_BITRATE_I_160                      5
#define FLO_SYNTAX_MPEG_BITRATE_I_192                      6
#define FLO_SYNTAX_MPEG_BITRATE_I_224                      7
#define FLO_SYNTAX_MPEG_BITRATE_I_256                      8
#define FLO_SYNTAX_MPEG_BITRATE_I_288                      9
#define FLO_SYNTAX_MPEG_BITRATE_I_320                      10
#define FLO_SYNTAX_MPEG_BITRATE_I_352                      11
#define FLO_SYNTAX_MPEG_BITRATE_I_384                      12
#define FLO_SYNTAX_MPEG_BITRATE_I_416                      13
#define FLO_SYNTAX_MPEG_BITRATE_I_448                      14

#define FLO_SYNTAX_MPEG_BITRATE_II_32                      1
#define FLO_SYNTAX_MPEG_BITRATE_II_48                      2
#define FLO_SYNTAX_MPEG_BITRATE_II_56                      3
#define FLO_SYNTAX_MPEG_BITRATE_II_64                      4
#define FLO_SYNTAX_MPEG_BITRATE_II_80                      5
#define FLO_SYNTAX_MPEG_BITRATE_II_96                      6
#define FLO_SYNTAX_MPEG_BITRATE_II_112                     7
#define FLO_SYNTAX_MPEG_BITRATE_II_128                     8
#define FLO_SYNTAX_MPEG_BITRATE_II_160                     9
#define FLO_SYNTAX_MPEG_BITRATE_II_192                     10
#define FLO_SYNTAX_MPEG_BITRATE_II_224                     11
#define FLO_SYNTAX_MPEG_BITRATE_II_256                     12
#define FLO_SYNTAX_MPEG_BITRATE_II_320                     13
#define FLO_SYNTAX_MPEG_BITRATE_II_384                     14

#define FLO_SYNTAX_MPEG_BITRATE_III_32                     1
#define FLO_SYNTAX_MPEG_BITRATE_III_40                     2
#define FLO_SYNTAX_MPEG_BITRATE_III_48                     3
#define FLO_SYNTAX_MPEG_BITRATE_III_56                     4
#define FLO_SYNTAX_MPEG_BITRATE_III_64                     5
#define FLO_SYNTAX_MPEG_BITRATE_III_80                     6
#define FLO_SYNTAX_MPEG_BITRATE_III_96                     7
#define FLO_SYNTAX_MPEG_BITRATE_III_112                    8
#define FLO_SYNTAX_MPEG_BITRATE_III_128                    9
#define FLO_SYNTAX_MPEG_BITRATE_III_160                    10
#define FLO_SYNTAX_MPEG_BITRATE_III_192                    11
#define FLO_SYNTAX_MPEG_BITRATE_III_224                    12
#define FLO_SYNTAX_MPEG_BITRATE_III_256                    13
#define FLO_SYNTAX_MPEG_BITRATE_III_320                    14

#define FLO_SYNTAX_MPEG_BITRATE_ILLEGAL                    15

#define FLO_SYNTAX_MPEG_LAYER_III_BLOCK_TYPE_RESERVED      0
#define FLO_SYNTAX_MPEG_LAYER_III_BLOCK_TYPE_START_BLOCK   1
#define FLO_SYNTAX_MPEG_LAYER_III_BLOCK_TYPE_SHORT_WINDOWS 2
#define FLO_SYNTAX_MPEG_LAYER_III_BLOCK_TYPE_END_BLOCK     3
#define FLO_SYNTAX_MPEG_LAYER_III_NB_FREQUENCY_LINES       576

#define FLO_SYNTAX_MPEG_LAYER_I_PCM_SAMPLES_PER_FRAME      384
#define FLO_SYNTAX_MPEG_LAYER_II_PCM_SAMPLES_PER_FRAME     1152
#define FLO_SYNTAX_MPEG_LAYER_III_MPEG1_PCM_SAMPLES_PER_FRAME    1152
#define FLO_SYNTAX_MPEG_LAYER_III_MPEG2_PCM_SAMPLES_PER_FRAME    576

#endif /* _FLO_SYNTAX_H_ */
