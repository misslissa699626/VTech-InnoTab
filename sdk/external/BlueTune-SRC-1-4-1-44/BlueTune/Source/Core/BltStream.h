/*****************************************************************
|
|   BlueTune - Stream Interface
|
|   (c) 2002-2009 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BLT_Stream interface
 */

#ifndef _BLT_STREAM_H_
#define _BLT_STREAM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltDefs.h"
#include "BltTypes.h"
#include "BltErrors.h"
#include "BltTime.h"

/*----------------------------------------------------------------------
|   error codes
+---------------------------------------------------------------------*/
#define BLT_ERROR_STREAM_NO_COMPATIBLE_NODE (BLT_ERROR_BASE_STREAM - 0)
#define BLT_ERROR_STREAM_INPUT_NOT_FOUND    (BLT_ERROR_BASE_STREAM - 1)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_STREAM_INFO_MASK_ALL             0x1FFF

#define BLT_STREAM_INFO_MASK_TYPE            0x0001
#define BLT_STREAM_INFO_MASK_ID              0x0002
#define BLT_STREAM_INFO_MASK_NOMINAL_BITRATE 0x0004
#define BLT_STREAM_INFO_MASK_AVERAGE_BITRATE 0x0008
#define BLT_STREAM_INFO_MASK_INSTANT_BITRATE 0x0010
#define BLT_STREAM_INFO_MASK_SIZE            0x0020
#define BLT_STREAM_INFO_MASK_DURATION        0x0040
#define BLT_STREAM_INFO_MASK_SAMPLE_RATE     0x0080
#define BLT_STREAM_INFO_MASK_CHANNEL_COUNT   0x0100
#define BLT_STREAM_INFO_MASK_WIDTH           0x0200
#define BLT_STREAM_INFO_MASK_HEIGHT          0x0400
#define BLT_STREAM_INFO_MASK_FLAGS           0x0800
#define BLT_STREAM_INFO_MASK_DATA_TYPE       0x1000

/** Flag that indicates that the stream has variable bitrate */
#define BLT_STREAM_INFO_FLAG_VBR             0x01

/** Flag that indicates that the stream is a continuous stream */
#define BLT_STREAM_INFO_FLAG_CONTINUOUS      0x02

#define BLT_SEEK_POINT_MASK_TIME_STAMP       0x01
#define BLT_SEEK_POINT_MASK_POSITION         0x02
#define BLT_SEEK_POINT_MASK_OFFSET           0x04
#define BLT_SEEK_POINT_MASK_SAMPLE           0x08

#define BLT_STREAM_NODE_FLAG_TRANSIENT 1

#define BLT_STREAM_AUDIO_TRACK_SELECTOR_INDEX_PROPERTY  "Audio.TrackSelector.Index"
#define BLT_STREAM_AUDIO_TRACK_SELECTOR_ID_PROPERTY     "Audio.TrackSelector.ID"
#define BLT_STREAM_AUDIO_TRACK_SELECTOR_STRICT_PROPERTY "Audio.TrackSelector.Strict"

#define BLT_STREAM_VIDEO_TRACK_SELECTOR_INDEX_PROPERTY  "Video.TrackSelector.Index"
#define BLT_STREAM_VIDEO_TRACK_SELECTOR_ID_PROPERTY     "Video.TrackSelector.ID"
#define BLT_STREAM_VIDEO_TRACK_SELECTOR_STRICT_PROPERTY "Video.TrackSelector.Strict"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef enum {
    BLT_STREAM_TYPE_UNKNOWN,
    BLT_STREAM_TYPE_MULTIPLEXED,
    BLT_STREAM_TYPE_AUDIO,
    BLT_STREAM_TYPE_VIDEO
} BLT_StreamType;

typedef struct {
    BLT_Mask       mask;             /**< Mask indicating which fields are valid */
    BLT_StreamType type;             /**< Stream Type                            */
    BLT_UInt32     id;               /**< Stream ID                              */
    BLT_UInt32     nominal_bitrate;  /**< Nominal bitrate                        */
    BLT_UInt32     average_bitrate;  /**< Average bitrate                        */
    BLT_UInt32     instant_bitrate;  /**< Instant bitrate                        */
    BLT_UInt64     size;             /**< Size in bytes                          */
    BLT_UInt64     duration;         /**< Duration in milliseconds               */
    BLT_UInt32     sample_rate;      /**< Sample rate in Hz                      */
    BLT_UInt16     channel_count;    /**< Number of channels                     */
    BLT_UInt16     width;            /**< Picture Width                          */
    BLT_UInt16     height;           /**< Picture Height                         */
    BLT_Flags      flags;            /**< Stream Flags                           */
    BLT_CString    data_type;        /**< Human-readable data type               */
} BLT_StreamInfo;

typedef enum {
    BLT_SEEK_MODE_IGNORE,
    BLT_SEEK_MODE_BY_TIME_STAMP,
    BLT_SEEK_MODE_BY_POSITION,
    BLT_SEEK_MODE_BY_OFFSET,
    BLT_SEEK_MODE_BY_SAMPLE
} BLT_SeekMode;

typedef struct {
    BLT_UInt64 offset;       /**< Offset from start (between 0 and range) */
    BLT_UInt64 range;        /**< Range of possible offsets               */
} BLT_StreamPosition;

typedef struct {
    BLT_Mask           mask;       /**< Mask indicating valid fields         */
    BLT_TimeStamp      time_stamp; /**< Time stamp (seconds,nanoseconds)     */
    BLT_StreamPosition position;   /**< Position (offset,range)              */
    BLT_Position       offset;     /**< Absolute offset in bytes             */
    ATX_Int64          sample;
} BLT_SeekPoint;

/*----------------------------------------------------------------------
|   more includes
+---------------------------------------------------------------------*/
#include "BltEventListener.h"
#include "BltModule.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMediaPort.h"
#include "BltOutputNode.h"

/*----------------------------------------------------------------------
|   more types
+---------------------------------------------------------------------*/
typedef struct {
    BLT_MediaNode* media_node;
    BLT_Flags      flags;
    struct {
        BLT_Boolean           connected;
        BLT_MediaPortProtocol protocol;
    } input;
    struct {
        BLT_Boolean           connected;
        BLT_MediaPortProtocol protocol;
    } output;
} BLT_StreamNodeInfo;

typedef struct {
    BLT_StreamPosition   position;
    BLT_TimeStamp        time_stamp;
    BLT_OutputNodeStatus output_status;
} BLT_StreamStatus;

/*----------------------------------------------------------------------
|   BLT_Stream Interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_DEFINITION(BLT_Stream)
    BLT_Result (*SetEventListener)(BLT_Stream*        self,
                                   BLT_EventListener* listener);
    BLT_Result (*ResetInput)(BLT_Stream* self);
    BLT_Result (*SetInput)(BLT_Stream* self, 
                           BLT_CString name,
                           BLT_CString type);
    BLT_Result (*SetInputNode)(BLT_Stream*    self,
                               BLT_CString    name,
                               BLT_CString    port,
                               BLT_MediaNode* node);
    BLT_Result (*GetInputNode)(BLT_Stream* self, BLT_MediaNode** node);
    BLT_Result (*ResetOutput)(BLT_Stream* self);
    BLT_Result (*SetOutput)(BLT_Stream* self, 
                            BLT_CString name,
                            BLT_CString type);
    BLT_Result (*SetOutputNode)(BLT_Stream*    self,
				                BLT_CString    name,
                                BLT_MediaNode* node);
    BLT_Result (*GetOutputNode)(BLT_Stream* stream, BLT_MediaNode** node);
    BLT_Result (*AddNode)(BLT_Stream*    self, 
                          BLT_MediaNode* where,
                          BLT_MediaNode* node);
    BLT_Result (*AddNodeByName)(BLT_Stream*    self, 
                                BLT_MediaNode* where,
                                BLT_CString    name);
    BLT_Result (*GetStreamNodeInfo)(BLT_Stream*          self,
                                    const BLT_MediaNode* node,
                                    BLT_StreamNodeInfo*  info);
    BLT_Result (*GetFirstNode)(BLT_Stream*         self,
                               BLT_MediaNode**     node);
    BLT_Result (*GetNextNode)(BLT_Stream*         self,
                              BLT_MediaNode*      node,
                              BLT_MediaNode**     next);
    BLT_Result (*PumpPacket)(BLT_Stream* self);
    BLT_Result (*Start)(BLT_Stream* self);
    BLT_Result (*Stop)(BLT_Stream* self);
    BLT_Result (*Pause)(BLT_Stream* self);
    BLT_Result (*SetInfo)(BLT_Stream* self, const BLT_StreamInfo* info);
    BLT_Result (*GetInfo)(BLT_Stream* self, BLT_StreamInfo* info);
    BLT_Result (*GetStatus)(BLT_Stream* self, BLT_StreamStatus* status);
    BLT_Result (*GetProperties)(BLT_Stream* self, ATX_Properties** settings);
    BLT_Result (*EstimateSeekPoint)(BLT_Stream*    self,
                                    BLT_SeekMode   mode,
                                    BLT_SeekPoint* point);
    BLT_Result (*SeekToTime)(BLT_Stream* self, BLT_UInt64 time);
    BLT_Result (*SeekToPosition)(BLT_Stream* self,
                                 BLT_UInt64  offset,
                                 BLT_UInt64  range);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define BLT_Stream_SetEventListener(object, listener) \
ATX_INTERFACE(object)->SetEventListener(object, listener)

#define BLT_Stream_ResetInput(object) \
ATX_INTERFACE(object)->ResetInput(object)

#define BLT_Stream_SetInput(object, name, media_type) \
ATX_INTERFACE(object)->SetInput(object, name, media_type)

#define BLT_Stream_SetInputNode(object, name, port, node) \
ATX_INTERFACE(object)->SetInputNode(object, name, port, node)

#define BLT_Stream_GetInputNode(object, node) \
ATX_INTERFACE(object)->GetInputNode(object, node)

#define BLT_Stream_ResetOutput(object) \
ATX_INTERFACE(object)->ResetOutput(object)

#define BLT_Stream_SetOutput(object, name, media_type) \
ATX_INTERFACE(object)->SetOutput(object, name, media_type)

#define BLT_Stream_SetOutputNode(object, name, node) \
ATX_INTERFACE(object)->SetOutputNode(object, name, node)

#define BLT_Stream_GetOutputNode(object, node) \
ATX_INTERFACE(object)->GetOutputNode(object, node)

#define BLT_Stream_AddNode(object, where, node) \
ATX_INTERFACE(object)->AddNode(object, where, node)

#define BLT_Stream_AddNodeByName(object, where, name) \
ATX_INTERFACE(object)->AddNodeByName(object, where, name)

#define BLT_Stream_GetStreamNodeInfo(object, node, info) \
ATX_INTERFACE(object)->GetStreamNodeInfo(object, node, info)

#define BLT_Stream_GetFirstNode(object, node) \
ATX_INTERFACE(object)->GetFirstNode(object, node)

#define BLT_Stream_GetNextNode(object, node, next) \
ATX_INTERFACE(object)->GetNextNode(object, node, next)

#define BLT_Stream_PumpPacket(object) \
ATX_INTERFACE(object)->PumpPacket(object)

#define BLT_Stream_Start(object) \
ATX_INTERFACE(object)->Start(object)

#define BLT_Stream_Stop(object) \
ATX_INTERFACE(object)->Stop(object)

#define BLT_Stream_Pause(object) \
ATX_INTERFACE(object)->Pause(object)

#define BLT_Stream_SetInfo(object, info) \
ATX_INTERFACE(object)->SetInfo(object, info)

#define BLT_Stream_GetInfo(object, info) \
ATX_INTERFACE(object)->GetInfo(object, info)

#define BLT_Stream_GetStatus(object, status) \
ATX_INTERFACE(object)->GetStatus(object, status)

#define BLT_Stream_GetProperties(object, properties) \
ATX_INTERFACE(object)->GetProperties(object, properties)

#define BLT_Stream_EstimateSeekPoint(object, mode, point) \
ATX_INTERFACE(object)->EstimateSeekPoint(object, mode, point)

#define BLT_Stream_SeekToTime(object, time) \
ATX_INTERFACE(object)->SeekToTime(object, time)

#define BLT_Stream_SeekToPosition(object, offset, range) \
ATX_INTERFACE(object)->SeekToPosition(object, offset, range)

#endif /* _BLT_STREAM_H_ */
