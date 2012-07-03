/*****************************************************************
|
|   BlueTune - Events Definitions
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Events
 */

#ifndef _BLT_EVENT_H_
#define _BLT_EVENT_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltDefs.h"
#include "BltTypes.h"
#include "BltErrors.h"
#include "BltMediaNode.h"
#include "BltStream.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef enum {
    BLT_EVENT_TYPE_USER,
    BLT_EVENT_TYPE_INFO,
    BLT_EVENT_TYPE_DEBUG,
    BLT_EVENT_TYPE_STREAM_TOPOLOGY,
    BLT_EVENT_TYPE_STREAM_INFO,
    BLT_EVENT_TYPE_DECODING_ERROR
} BLT_EventType;

typedef struct BLT_Event BLT_Event;

typedef struct {
    BLT_Ordinal id;
    BLT_Size    size;
    /* payload follows (size bytes) */
} BLT_UserEvent;

typedef struct {
    BLT_CString message;
} BLT_InfoEvent;

typedef struct {
    BLT_Ordinal  category;
    BLT_Ordinal  level;
    BLT_CString  message;
} BLT_DebugEvent;

typedef enum {
    BLT_STREAM_TOPOLOGY_NODE_ADDED,
    BLT_STREAM_TOPOLOGY_NODE_REMOVED,
    BLT_STREAM_TOPOLOGY_NODE_CONNECTED,
    BLT_STREAM_TOPOLOGY_NODE_DISCONNECTED
} BLT_StreamTopologyEventType;

typedef struct {
    BLT_StreamTopologyEventType type;
    BLT_MediaNode*              node;
} BLT_StreamTopologyEvent;

typedef struct {
    BLT_Mask       update_mask;
    BLT_StreamInfo info;
} BLT_StreamInfoEvent;

typedef struct {
    BLT_Result  result;
    BLT_CString message;
} BLT_DecodingErrorEvent;

#endif /* _BLT_EVENT_H_ */
