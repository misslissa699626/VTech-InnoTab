/*****************************************************************
|
|   ReplayGain common definitions
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * ReplayGain API
 */

#ifndef _BLT_REPLAY_GAIN_H_
#define _BLT_REPLAY_GAIN_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "BltTypes.h"
#include "BltModule.h"
#include "BltStream.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef enum {
    BLT_REPLAY_GAIN_SET_MODE_UPDATE,
    BLT_REPLAY_GAIN_SET_MODE_REMOVE,
    BLT_REPLAY_GAIN_SET_MODE_IGNORE
} BLT_ReplayGainSetMode;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_REPLAY_GAIN_TRACK_GAIN_VALUE  "ReplayGain.TrackGain"
#define BLT_REPLAY_GAIN_TRACK_PEAK_VALUE  "ReplayGain.TrackPeak"
#define BLT_REPLAY_GAIN_ALBUM_GAIN_VALUE  "ReplayGain.AlbumGain"
#define BLT_REPLAY_GAIN_ALBUM_PEAK_VALUE  "ReplayGain.AlbumPeak"

#define BLT_VORBIS_COMMENT_REPLAY_GAIN_TRACK_GAIN "REPLAYGAIN_TRACK_GAIN"
#define BLT_VORBIS_COMMENT_REPLAY_GAIN_ALBUM_GAIN "REPLAYGAIN_ALBUM_GAIN"

/*----------------------------------------------------------------------
|   functions
+---------------------------------------------------------------------*/
BLT_Result
BLT_ReplayGain_SetStreamProperties(BLT_Stream*           stream,
                                   float                 track_gain,
                                   BLT_ReplayGainSetMode track_gain_mode,
                                   float                 album_gain,
                                   BLT_ReplayGainSetMode album_gain_mode);

#endif /* _BLT_REPLAY_GAIN_H_ */
