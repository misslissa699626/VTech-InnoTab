/*****************************************************************
|
|   Common Media Types
|
|   (c) 2002-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Common Media Types
 */

#ifndef _BLT_COMMON_MEDIA_TYPES_H_
#define _BLT_COMMON_MEDIA_TYPES_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "BltTypes.h"
#include "BltModule.h"
#include "BltStream.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_MP4_AUDIO_ES_MIME_TYPE      "audio/vnd.bluetune.mp4-es"
#define BLT_MP4_VIDEO_ES_MIME_TYPE      "video/vnd.bluetune.mp4-es"
#define BLT_ISO_BASE_AUDIO_ES_MIME_TYPE "audio/vnd.bluetune.iso-base-es"
#define BLT_ISO_BASE_VIDEO_ES_MIME_TYPE "video/vnd.bluetune.iso-base-es"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef enum {
    BLT_MP4_STREAM_TYPE_UNKNOWN,
    BLT_MP4_STREAM_TYPE_AUDIO,
    BLT_MP4_STREAM_TYPE_VIDEO
} BLT_Mp4StreamType;

/** 
 * Base type for mp4-based streams. The field stream_type indicates
 * that this struct is the base part of one of the subtypes defined
 * below: 
 * If stream_type is BLT_MP4_STREAM_TYPE_AUDIO, this is the base part 
 * of a BLT_Mp4AudioMediaType struct.
 * If stream_type is BLT_MP4_STREAM_TYPE_VIDEO, this is the base part 
 * of a BLT_Mp4VideoMediaType struct.
 * If stream_type is BLT_MP4_STREAM_TYPE_UNKNOWN, this struct is not
 * the base of any know subtype.
 */
typedef struct {
    BLT_MediaType     base;
    BLT_Mp4StreamType stream_type;
    BLT_UInt32        format_or_object_type_id;
} BLT_Mp4MediaType;

/** 
 * When the BLT_Mp4MediaType.stream_type field is BLT_MP4_STREAM_TYPE_AUDIO,
 * the full struct is the following.
 */
typedef struct {
    BLT_Mp4MediaType base;
    BLT_UInt32       sample_rate;
    BLT_UInt16       sample_size;
    BLT_UInt16       channel_count;
	BLT_UInt32       frame_count;
    unsigned int     decoder_info_length;
    /* variable size array follows */
    unsigned char    decoder_info[1]; /* could be more than 1 byte */
} BLT_Mp4AudioMediaType;

/** 
 * When the BLT_Mp4MediaType.stream_type field is BLT_MP4_STREAM_TYPE_AUDIO,
 * the full struct is the following.
 */
typedef struct {
    BLT_Mp4MediaType base;
    BLT_UInt16       width;
    BLT_UInt16       height;
    BLT_UInt16       depth;
    unsigned int     decoder_info_length;
    /* variable size array follows */
    unsigned char    decoder_info[1]; /* could be more than 1 byte */
} BLT_Mp4VideoMediaType;

#endif /* _BLT_COMMON_MEDIA_TYPES_H_ */
